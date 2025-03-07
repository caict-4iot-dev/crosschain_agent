
#ifndef PRIVATE_KEY_H_
#define PRIVATE_KEY_H_

#include <utils/headers.h>
#include <ed25519-donna/ed25519.h>
#include <utils/ecc_sm2.h>
#include <json/value.h>

namespace agent {
	typedef unsigned char sm2_public_key[65];
	typedef const unsigned char* puchar;
	enum SignatureType {
		SIGNTYPE_NONE,
		SIGNTYPE_ED25519 = 'e',
		SIGNTYPE_CFCASM2 = 'z',
		SIGNTYPE_RSA = 3,
		SIGNTYPE_CFCA = 4
	};

	enum EncodeType {
		ENCODETYPE_NONE,
		ENCODETYPE_BASE58 = 'f',
		ENCODETYPE_BASE64 = 's',
		ENCODETYPE_BECH32 = 't'
	};

	enum KeyType {
		KEY_ADDRESS = 0xa0, //0xa0
		KEY_PUBLICKEY = 0xb0, //0xb0
		KEY_PRIVATEKEY = 0xc0  //0xc0
	};

	enum Ed25519KeyLength {
		ED25519_ADDRESS_LENGTH = 22, // 2+1+22+4
		ED25519_PUBLICKEY_LENGTH = 32, //1+1+32+4
		ED25519_PRIVATEKEY_LENGTH = 32, //3+1+32+1+4
	};

	enum Sm2KeyLength {
		SM2_ADDRESS_LENGTH = 22, //2+1+22+4
		SM2_PUBLICKEY_LENGTH = 65, //1+1+65+4
		SM2_PRIVATEKEY_LENGTH = 32 //3+1+32+1+4
	};

	enum KeypairVersion {
		KEYPAIR_VERSION_V3 = 0,
		KEYPAIR_VERSION_V4,
	};

	enum SystemContractAddress {
		SCONTRACT_RESERVER = 0,
		SCONTRACT_CHAIN_MANAGER = 1,
		SCONTRACT_DPOS = 2,
		SCONTRACT_DEPLOY = 3,
		SCONTRACT_AUTH = 4,
		SCONTRACT_PRIVACY_PROTECT = 5,
		SCONTRACT_CROSS_CHAIN = 6,
		SCONTRACT_DDO = 7,
		SCONTRACT_CONTROLLED_AREA = 8,
		SCONTRACT_IDENT = 9,
        SCONTRACT_BLACK_LIST = 13,

        SCONTRACT_MAX_SIZE = 20,

        SCONTRACT_SIZE = 40,
	};


	//tools
	class KeypairUtils {
	public:
		//Init key pair version

		static std::string EncodeAddress(SignatureType stype, EncodeType etype, const std::string& address);
		static std::string DecodeAddress(const std::string& address);
		static std::string GetAddressIndex(const std::string& address);
		static std::string EncodePublicKey(const std::string& key);
		static std::string DecodePublicKey(const std::string& key);
		static std::string EncodePrivateKey(const std::string& key);
		static std::string DecodePrivateKey(const std::string& key);
		static std::string CalcHash(const std::string &value, const SignatureType &sign_type);
		static bool GetPublicKeyElement(const std::string &encode_pub_key, KeyType &key_type_tmp, SignatureType &sign_type, EncodeType &encode_type, std::string &raw_data);
		static bool GetKeyElement(const std::string &encode_key, KeyType &key_type_tmp, SignatureType &sign_type, EncodeType &encode_type, std::string &raw_data);
		static std::string GetSignTypeDesc(SignatureType type);
		static SignatureType GetSignTypeByDesc(const std::string &desc);


	private:
		static uint32_t keypair_version_;
	};

	//base 
	class PublicKeyBase {
	public:
		PublicKeyBase(bool valid, SignatureType type) : valid_(valid), type_(type){}
		~PublicKeyBase(){}

		virtual void Init(SignatureType type, std::string rawpkey) = 0;
		virtual std::string GetEncAddress() const = 0;
		virtual std::string GetEncPublicKey() const = 0;

		std::string GetRawPublicKey() const { return raw_pub_key_; };
		std::string GetRawAddress() const { return raw_address_; };
		bool IsValid() const { return valid_; }
		SignatureType GetSignType() const { return type_; };

	protected:
		bool valid_;
		SignatureType type_;
		EncodeType encode_type_;
		std::string raw_pub_key_;
		std::string raw_address_;
	};

	class PrivateKeyBase {
	public:
		PrivateKeyBase() : valid_(false), type_(SIGNTYPE_ED25519){}
		~PrivateKeyBase(){}

		virtual void Init(SignatureType type, std::string raw_priv_key) = 0;
		virtual bool From(const std::string &encode_private_key) = 0;
		virtual std::string	Sign(const std::string &input) const = 0;
		virtual std::string GetEncPrivateKey() const = 0;

		std::string GetRawPrivateKey() const{ return KeypairUtils::EncodePrivateKey(raw_priv_key_); }
		std::string GetRawHexPrivateKey() const { return utils::String::BinToHexString(raw_priv_key_); }
		bool IsValid() const { return valid_; }
		SignatureType GetSignType() const { return type_; };
		std::string GetEncAddress() const { return pub_key_->GetEncAddress(); };
		std::string GetRawAddress() const { return pub_key_->GetRawAddress(); };
		std::string GetEncPublicKey() const { return pub_key_->GetEncPublicKey(); };
		std::string GetRawPublicKey() const { return pub_key_->GetRawPublicKey(); };

	protected:
		bool valid_;
		SignatureType type_;
		EncodeType encode_type_;
		std::string raw_priv_key_;
		static utils::Mutex lock_;
		std::shared_ptr<PublicKeyBase> pub_key_;
	};

	//v4 
	class PublicKeyV4 : public PublicKeyBase {
		DISALLOW_COPY_AND_ASSIGN(PublicKeyV4);
		friend class PrivateKeyV4;

	public:
		PublicKeyV4();
		PublicKeyV4(bool valid, SignatureType type);
		PublicKeyV4(const std::string &encode_pub_key);
		~PublicKeyV4(){}

		void Init(SignatureType type, std::string rawpkey);

		std::string GetEncAddress() const;

		std::string GetEncPublicKey() const;

		static bool Verify(const std::string &data, const std::string &signature, const std::string &encode_public_key);
	};

	class PrivateKeyV4 : public PrivateKeyBase {
		DISALLOW_COPY_AND_ASSIGN(PrivateKeyV4);
	public:
		PrivateKeyV4(SignatureType type);
		PrivateKeyV4(const std::string &encode_private_key);
		bool From(const std::string &encode_private_key);
		~PrivateKeyV4(){}

		void Init(SignatureType type, std::string raw_priv_key);

		std::string	Sign(const std::string &input) const;
		std::string GetEncPrivateKey() const;
	};

	//wrapper
	class PublicKey {
		DISALLOW_COPY_AND_ASSIGN(PublicKey);
		friend class PrivateKey;

	public:
		PublicKey();
		PublicKey(const std::string &encode_pub_key);
		~PublicKey();

		void Init(SignatureType type, std::string rawpkey);

		std::string GetEncAddress() const;
		std::string GetRawAddress() const;
		std::string GetEncPublicKey() const;
		std::string GetRawPublicKey() const;

		bool IsValid() const;

		SignatureType GetSignType() const;

		static bool Verify(const std::string &data, const std::string &signature, const std::string &encode_public_key);
		static std::string EncodeAddress(const std::string &prefix, const std::string &raw_address);

	private:
		std::shared_ptr<PublicKeyBase> pub_base_;
	};

	class PrivateKey {
		DISALLOW_COPY_AND_ASSIGN(PrivateKey);
	public:
		PrivateKey(SignatureType type);
		PrivateKey(const std::string &encode_private_key);
		bool From(const std::string &encode_private_key);
		~PrivateKey();

		void Init(SignatureType type, std::string raw_priv_key);

		std::string	Sign(const std::string &input) const;
		std::string GetEncPrivateKey() const;
		std::string GetEncAddress() const;
		std::string GetRawAddress() const;
		std::string GetEncPublicKey() const;
		std::string GetRawPublicKey() const;
		std::string GetRawHexPrivateKey() const;
		bool IsValid() const;
		std::string GetRawPrivateKey() const;
		SignatureType GetSignType() const;

	private:
		std::shared_ptr<PrivateKeyBase> priv_base_;
	};

    class HashWrapper : public utils::NonCopyable {
		int32_t type_;// 0 : protocol::LedgerUpgrade::SHA256, 1: protocol::LedgerUpgrade::SM3
		utils::Hash *hash_;
	public:
		enum HashType {
			HASH_TYPE_SHA256 = 0,
			HASH_TYPE_SM3 = 1,
			HASH_TYPE_MAX = 2
		};

		HashWrapper();
		HashWrapper(int32_t type);
		~HashWrapper();

		void Update(const std::string &input);
		void Update(const void *buffer, size_t len);
		std::string Final();

		static void SetLedgerHashType(int32_t type);
		static int32_t GetLedgerHashType();

		static std::string Crypto(const std::string &input);
		static void Crypto(unsigned char* str, int len, unsigned char *buf);
		static void Crypto(const std::string &input, std::string &str);
	};
};

#endif
