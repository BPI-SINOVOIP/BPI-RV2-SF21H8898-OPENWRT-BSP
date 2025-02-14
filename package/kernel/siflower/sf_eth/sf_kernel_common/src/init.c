/*
* Description
*
* Copyright (C) 2016-2023 Qin.Xia <qin.xia@siflower.com.cn>
*
* Siflower software
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/parser.h>
#include <linux/string.h>
#include <linux/string_helpers.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "sf_kernel_common.h"

extern int fast_ping_probe(void);
extern void fast_ping_remove(void);

struct ddebug_table {
	struct list_head link;
	const char *mod_name;
	unsigned int num_ddebugs;
	struct _ddebug *ddebugs;
};

struct ddebug_query {
	const char *filename;
	const char *module;
	const char *function;
	const char *format;
	unsigned int first_lineno, last_lineno;
};

struct ddebug_iter {
	struct ddebug_table *table;
	unsigned int idx;
};


static DEFINE_MUTEX(ddebug_lock);
static LIST_HEAD(ddebug_tables);


static inline const char *trim_prefix(const char *path)
{
	int len = strlen(path);
	int i, skip = 0;

	/* skip to the last file name */
	for (i = 0; i < len; i++) {
		if (*(path + i) == '/')
			skip = i + 1;
	}

	return path + skip;
}

static void vpr_info_dq(const struct ddebug_query *query, const char *msg)
{
	/* trim any trailing newlines */
	int fmtlen = 0;

	if (query->format) {
		fmtlen = strlen(query->format);
		while (fmtlen && query->format[fmtlen - 1] == '\n')
			fmtlen--;
	}

	pr_debug("%s: func=\"%s\" file=\"%s\" module=\"%s\" format=\"%.*s\" lineno=%u-%u\n",
		 msg,
		 query->function ?: "",
		 query->filename ?: "",
		 query->module ?: "",
		 fmtlen, query->format ?: "",
		 query->first_lineno, query->last_lineno);
}

static int ddebug_tokenize(char *buf, char *words[], int maxwords)
{
	int nwords = 0;

	while (*buf) {
		char *end;

		/* Skip leading whitespace */
		buf = skip_spaces(buf);
		if (!*buf)
			break;	/* oh, it was trailing whitespace */
		if (*buf == '#')
			break;	/* token starts comment, skip rest of line */

		/* find `end' of word, whitespace separated or quoted */
		if (*buf == '"' || *buf == '\'') {
			int quote = *buf++;
			for (end = buf; *end && *end != quote; end++)
				;
			if (!*end) {
				pr_err("unclosed quote: %s\n", buf);
				return -EINVAL;	/* unclosed quote */
			}
		} else {
			for (end = buf; *end && !isspace(*end); end++)
				;
			BUG_ON(end == buf);
		}

		/* `buf' is start of word, `end' is one past its end */
		if (nwords == maxwords) {
			pr_err("too many words, legal max <=%d\n", maxwords);
			return -EINVAL;	/* ran out of words[] before bytes */
		}
		if (*end)
			*end++ = '\0';	/* terminate the word */
		words[nwords++] = buf;
		buf = end;
	}

	return nwords;
}

static int ddebug_parse_flags(const char *str, int *change)
{
	int op;

	switch (*str) {
	case '+':
	case '-':
		op = *str++;
		break;
	default:
		pr_err("bad flag-op %c, at start of %s\n", *str, str);
		return -EINVAL;
	}

	switch (op) {
	case '+':
		*change = 1;
		break;
	case '-':
		*change = 0;
		break;
	}

	return 0;
}

static inline int parse_lineno(const char *str, unsigned int *val)
{
	BUG_ON(str == NULL);
	if (*str == '\0') {
		*val = 0;
		return 0;
	}
	if (kstrtouint(str, 10, val) < 0) {
		pr_err("bad line-number: %s\n", str);
		return -EINVAL;
	}
	return 0;
}

static int parse_linerange(struct ddebug_query *query, const char *first)
{
	char *last = strchr(first, '-');

	if (query->first_lineno || query->last_lineno) {
		pr_err("match-spec: line used 2x\n");
		return -EINVAL;
	}
	if (last)
		*last++ = '\0';
	if (parse_lineno(first, &query->first_lineno) < 0)
		return -EINVAL;
	if (last) {
		/* range <first>-<last> */
		if (parse_lineno(last, &query->last_lineno) < 0)
			return -EINVAL;

		/* special case for last lineno not specified */
		if (query->last_lineno == 0)
			query->last_lineno = UINT_MAX;

		if (query->last_lineno < query->first_lineno) {
			pr_err("last-line:%d < 1st-line:%d\n",
			       query->last_lineno,
			       query->first_lineno);
			return -EINVAL;
		}
	} else {
		query->last_lineno = query->first_lineno;
	}
	pr_debug("parsed line %d-%d\n", query->first_lineno,
		 query->last_lineno);
	return 0;
}

static int check_set(const char **dest, char *src, char *name)
{
	int rc = 0;

	if (*dest) {
		rc = -EINVAL;
		pr_err("match-spec:%s val:%s overridden by %s\n",
		       name, *dest, src);
	}
	*dest = src;
	return rc;
}

static int ddebug_parse_query(char *words[], int nwords,
			struct ddebug_query *query, const char *modname)
{
	unsigned int i;
	int rc = 0;
	char *fline;

	/* check we have an even number of words */
	if (nwords % 2 != 0) {
		pr_err("expecting pairs of match-spec <value>\n");
		return -EINVAL;
	}

	if (modname)
		/* support $modname.dyndbg=<multiple queries> */
		query->module = modname;

	for (i = 0; i < nwords; i += 2) {
		char *keyword = words[i];
		char *arg = words[i+1];

		if (!strcmp(keyword, "func")) {
			rc = check_set(&query->function, arg, "func");
		} else if (!strcmp(keyword, "file")) {
			if (check_set(&query->filename, arg, "file"))
				return -EINVAL;

			/* tail :$info is function or line-range */
			fline = strchr(query->filename, ':');
			if (!fline)
				continue;
			*fline++ = '\0';
			if (isalpha(*fline) || *fline == '*' || *fline == '?') {
				/* take as function name */
				if (check_set(&query->function, fline, "func"))
					return -EINVAL;
			} else {
				if (parse_linerange(query, fline))
					return -EINVAL;
			}
		} else if (!strcmp(keyword, "module")) {
			rc = check_set(&query->module, arg, "module");
		} else if (!strcmp(keyword, "format")) {
			string_unescape_inplace(arg, UNESCAPE_SPACE |
							    UNESCAPE_OCTAL |
							    UNESCAPE_SPECIAL);
			rc = check_set(&query->format, arg, "format");
		} else if (!strcmp(keyword, "line")) {
			if (parse_linerange(query, arg))
				return -EINVAL;
		} else {
			pr_err("unknown keyword \"%s\"\n", keyword);
			return -EINVAL;
		}
		if (rc)
			return rc;
	}
	vpr_info_dq(query, "parsed");
	return 0;
}

static int ddebug_change(const struct ddebug_query *query, int change)
{
	int i;
	struct ddebug_table *dt;
	unsigned int nfound = 0;

	/* search for matching ddebugs */
	mutex_lock(&ddebug_lock);
	list_for_each_entry(dt, &ddebug_tables, link) {

		/* match against the module name */
		if (query->module &&
		    !match_wildcard(query->module, dt->mod_name))
			continue;

		for (i = 0; i < dt->num_ddebugs; i++) {
			struct _ddebug *dp = &dt->ddebugs[i];

			/* match against the source filename */
			if (query->filename &&
			    !match_wildcard(query->filename, dp->filename) &&
			    !match_wildcard(query->filename,
					   kbasename(dp->filename)))
				continue;

			/* match against the function */
			if (query->function &&
			    !match_wildcard(query->function, dp->function))
				continue;

			/* match against the format */
			if (query->format) {
				if (*query->format == '^') {
					char *p;
					/* anchored search. match must be at beginning */
					p = strstr(dp->format, query->format+1);
					if (p != dp->format)
						continue;
				} else if (!strstr(dp->format, query->format))
					continue;
			}

			/* match against the line number range */
			if (query->first_lineno &&
			    dp->lineno < query->first_lineno)
				continue;
			if (query->last_lineno &&
			    dp->lineno > query->last_lineno)
				continue;

			nfound++;

			if (change)
				atomic_set(&dp->enabled, 1);
			else
				atomic_set(&dp->enabled, 0);

			pr_debug("changed %s:%d [%s]%s =%d\n",
				 trim_prefix(dp->filename), dp->lineno,
				 dt->mod_name, dp->function,
				 change);
		}
	}
	mutex_unlock(&ddebug_lock);

	if (!nfound)
		pr_info("no matches for query\n");

	return nfound;
}

static int ddebug_exec_query(char *query_string, const char *modname)
{
	struct ddebug_query query = {};
#define MAXWORDS 9
	int nwords, nfound, change;
	char *words[MAXWORDS];

	nwords = ddebug_tokenize(query_string, words, MAXWORDS);
	if (nwords <= 0) {
		pr_err("tokenize failed\n");
		return -EINVAL;
	}
	/* check flags 1st (last arg) so query is pairs of spec,val */
	if (ddebug_parse_flags(words[nwords-1], &change)) {
		pr_err("flags parse failed\n");
		return -EINVAL;
	}
	if (ddebug_parse_query(words, nwords-1, &query, modname)) {
		pr_err("query parse failed\n");
		return -EINVAL;
	}
	/* actually go and implement the change */
	nfound = ddebug_change(&query, change);
	vpr_info_dq(&query, nfound ? "applied" : "no-match");

	return nfound;
}

static int ddebug_exec_queries(char *query, const char *modname)
{
	char *split;
	int i, errs = 0, exitcode = 0, rc, nfound = 0;

	for (i = 0; query; query = split) {
		split = strpbrk(query, ";\n");
		if (split)
			*split++ = '\0';

		query = skip_spaces(query);
		if (!query || !*query || *query == '#')
			continue;

		pr_debug("query %d: \"%s\"\n", i, query);

		rc = ddebug_exec_query(query, modname);
		if (rc < 0) {
			errs++;
			exitcode = rc;
		} else {
			nfound += rc;
		}
		i++;
	}
	pr_debug("processed %d queries, with %d matches, %d errs\n",
		 i, nfound, errs);

	if (exitcode)
		return exitcode;
	return nfound;
}

static int ddebug_add_module(struct _ddebug *tab, unsigned int n,
			     const char *name)
{
	struct ddebug_table *dt;

	dt = kzalloc(sizeof(*dt), GFP_KERNEL);
	if (dt == NULL) {
		pr_err("error adding module: %s\n", name);
		return -ENOMEM;
	}
	/*
	 * For built-in modules, name lives in .rodata and is
	 * immortal. For loaded modules, name points at the name[]
	 * member of struct module, which lives at least as long as
	 * this struct ddebug_table.
	 */
	dt->mod_name = name;
	dt->num_ddebugs = n;
	dt->ddebugs = tab;

	mutex_lock(&ddebug_lock);
	list_add(&dt->link, &ddebug_tables);
	mutex_unlock(&ddebug_lock);

	return 0;
}

static void ddebug_table_free(struct ddebug_table *dt)
{
	list_del_init(&dt->link);
	kfree(dt);
}

int sf_dynamic_debug_deinit(struct module *mod)
{
	struct ddebug_table *dt, *nextdt;
	int ret = -ENOENT;

	printk("removing module \"%s\"\n", mod->name);

	mutex_lock(&ddebug_lock);
	list_for_each_entry_safe(dt, nextdt, &ddebug_tables, link) {
		if (dt->mod_name == mod->name) {
			ddebug_table_free(dt);
			ret = 0;
			break;
		}
	}
	mutex_unlock(&ddebug_lock);
	return ret;
}
EXPORT_SYMBOL(sf_dynamic_debug_deinit);

static void ddebug_remove_all_tables(void)
{
	mutex_lock(&ddebug_lock);
	while (!list_empty(&ddebug_tables)) {
		struct ddebug_table *dt = list_entry(ddebug_tables.next,
						      struct ddebug_table,
						      link);
		ddebug_table_free(dt);
	}
	mutex_unlock(&ddebug_lock);
}

int sf_dynamic_debug_init(struct module *mod)
{
	struct module_notes_attrs *notes_attrs = NULL;
	struct bin_attribute *nattr;
	struct _ddebug *iter_start = NULL;
	int i, entries;
	int ret;

#ifdef CONFIG_KALLSYMS
	notes_attrs = mod->notes_attrs;
#endif
	if (!notes_attrs || notes_attrs->notes == 0) {
		printk("Ignore module:%s empty note section table\n", mod->name);
		return 0;
	}

	for (i = 0; i < notes_attrs->notes; i++) {
		nattr = (struct bin_attribute *)&notes_attrs->attrs[i];
		if (!strncmp(nattr->attr.name, ".note.sf_dyndbg", 15)) {
			iter_start = (struct _ddebug *)nattr->private;
			entries = nattr->size / sizeof(struct _ddebug);
			break;
		}
	}

	if (iter_start == NULL) {
		printk("Ignore module:%s empty ddebug table\n", mod->name);
		return 0;
	}


	ret = ddebug_add_module(iter_start, entries, mod->name);
	if (ret)
		return ret;

	printk("%s module %d entries %ld byte in _ddebug %ld bytes in sf_dyndbg section\n",
		 mod->name, entries, sizeof(struct _ddebug), nattr->size);
	return 0;
}
EXPORT_SYMBOL(sf_dynamic_debug_init);

/*
 * Set the iterator to point to the first _ddebug object
 * and return a pointer to that first object.  Returns
 * NULL if there are no _ddebugs at all.
 */
static struct _ddebug *ddebug_iter_first(struct ddebug_iter *iter)
{
	if (list_empty(&ddebug_tables)) {
		iter->table = NULL;
		iter->idx = 0;
		return NULL;
	}
	iter->table = list_entry(ddebug_tables.next,
				 struct ddebug_table, link);
	iter->idx = 0;
	return &iter->table->ddebugs[iter->idx];
}

/*
 * Advance the iterator to point to the next _ddebug
 * object from the one the iterator currently points at,
 * and returns a pointer to the new _ddebug.  Returns
 * NULL if the iterator has seen all the _ddebugs.
 */
static struct _ddebug *ddebug_iter_next(struct ddebug_iter *iter)
{
	if (iter->table == NULL)
		return NULL;
	if (++iter->idx == iter->table->num_ddebugs) {
		/* iterate to next table */
		iter->idx = 0;
		if (list_is_last(&iter->table->link, &ddebug_tables)) {
			iter->table = NULL;
			return NULL;
		}
		iter->table = list_entry(iter->table->link.next,
					 struct ddebug_table, link);
	}
	return &iter->table->ddebugs[iter->idx];
}

/*
 * Seq_ops start method.  Called at the start of every
 * read() call from userspace.  Takes the ddebug_lock and
 * seeks the seq_file's iterator to the given position.
 */
static void *ddebug_proc_start(struct seq_file *m, loff_t *pos)
{
	struct ddebug_iter *iter = m->private;
	struct _ddebug *dp;
	int n = *pos;

	mutex_lock(&ddebug_lock);

	if (!n)
		return SEQ_START_TOKEN;
	if (n < 0)
		return NULL;
	dp = ddebug_iter_first(iter);
	while (dp != NULL && --n > 0)
		dp = ddebug_iter_next(iter);
	return dp;
}

/*
 * Seq_ops next method.  Called several times within a read()
 * call from userspace, with ddebug_lock held.  Walks to the
 * next _ddebug object with a special case for the header line.
 */
static void *ddebug_proc_next(struct seq_file *m, void *p, loff_t *pos)
{
	struct ddebug_iter *iter = m->private;
	struct _ddebug *dp;

	if (p == SEQ_START_TOKEN)
		dp = ddebug_iter_first(iter);
	else
		dp = ddebug_iter_next(iter);
	++*pos;
	return dp;
}

/*
 * Seq_ops show method.  Called several times within a read()
 * call from userspace, with ddebug_lock held.  Formats the
 * current _ddebug as a single human-readable line, with a
 * special case for the header line.
 */
static int ddebug_proc_show(struct seq_file *m, void *p)
{
	struct ddebug_iter *iter = m->private;
	struct _ddebug *dp = p;

	if (p == SEQ_START_TOKEN) {
		seq_puts(m,
			 "# filename:lineno [module]function format\n");
		return 0;
	}

	seq_printf(m, "%s:%u [%s]%s =%d \"",
		   trim_prefix(dp->filename), dp->lineno,
		   iter->table->mod_name, dp->function,
		   atomic_read(&dp->enabled));
	seq_escape(m, dp->format, "\t\r\n\"");
	seq_puts(m, "\"\n");

	return 0;
}

/*
 * Seq_ops stop method.  Called at the end of each read()
 * call from userspace.  Drops ddebug_lock.
 */
static void ddebug_proc_stop(struct seq_file *m, void *p)
{
	mutex_unlock(&ddebug_lock);
}

static const struct seq_operations ddebug_proc_seqops = {
	.start = ddebug_proc_start,
	.next = ddebug_proc_next,
	.show = ddebug_proc_show,
	.stop = ddebug_proc_stop
};

static int ddebug_proc_open(struct inode *inode, struct file *file)
{
	return seq_open_private(file, &ddebug_proc_seqops,
				sizeof(struct ddebug_iter));
}

#define USER_BUF_PAGE 4096
static ssize_t ddebug_proc_write(struct file *file, const char __user *ubuf,
				  size_t len, loff_t *offp)
{
	char *tmpbuf;
	int ret;

	if (len == 0)
		return 0;
	if (len > USER_BUF_PAGE - 1) {
		pr_warn("expected <%d bytes into control\n", USER_BUF_PAGE);
		return -E2BIG;
	}
	tmpbuf = memdup_user_nul(ubuf, len);
	if (IS_ERR(tmpbuf))
		return PTR_ERR(tmpbuf);
	pr_debug("read %d bytes from userspace\n", (int)len);

	ret = ddebug_exec_queries(tmpbuf, NULL);
	kfree(tmpbuf);
	if (ret < 0)
		return ret;

	*offp += len;
	return len;
}

static const struct proc_ops proc_fops = {
	.proc_open = ddebug_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release_private,
	.proc_write = ddebug_proc_write
};

static int __init sf_kernel_common_init(void)
{
	int ret;

	proc_mkdir("sf_dynamic_debug", NULL);
	proc_create("sf_dynamic_debug/control", 0644, NULL, &proc_fops);
	ret = fast_ping_probe();

	return ret;
}

static void __exit sf_kernel_common_exit(void)
{
	fast_ping_remove();
	ddebug_remove_all_tables();

	remove_proc_subtree("sf_dynamic_debug", NULL);
}

module_init(sf_kernel_common_init);
module_exit(sf_kernel_common_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Qin Xia <qin.xia@siflower.com.cn>");
MODULE_DESCRIPTION("Siflower Driver");
