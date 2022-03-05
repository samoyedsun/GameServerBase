#ifndef W_CRYPTO_H
#define W_CRYPTO_H

#include "wrc4.h"

namespace wang {

	// RC4加密算法, 加密, 解密后数据长度不发生变化
	class wcrypto
	{
	public:
		wcrypto() {}
		virtual ~wcrypto() {}

		bool set_encrypt_key(const void* encrypt_data, int encrypt_len);
		bool set_decrypt_key(const void* decrypt_data, int decrypt_len);
		void encrypt(const void* data_in, void* data_out, int len_in);
		void decrypt(const void* data_in, void* data_out, int len_in);

	private:
		//RC4_KEY				m_encrypt_key;
		//RC4_KEY				m_decrypt_key;
		wRC4				m_encrypt_key;
		wRC4				m_decrypt_key;
	};

} // namespace wang

#endif //W_CRYPTO_H
