encrypt.c:给原始bin文件加header，加密。

encrypt_header.c:给原始bin文件加header，不加密。

encrypt_header_crc.c:给原始bin文件加header，不加密，加OTA相关header。主要测试用。

keys.txt:提供加密所需的AES_KEY，可修改。同时bootloader 解密部分也得一并修改。修改方法:替换文件aes_keys.c。