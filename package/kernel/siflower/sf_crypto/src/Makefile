obj-m += sf_crypto.o
sf_crypto-objs := core.o aes.o
sf_crypto-$(CONFIG_CRYPTO_SIFLOWER_HASH) += sha1.o sha256.o sha512.o md5.o
