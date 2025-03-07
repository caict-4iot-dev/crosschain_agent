

#include <openssl/ecdsa.h>
#include <openssl/rsa.h>
#include <openssl/ripemd.h>
#include <utils/logger.h>
#include <utils/crypto.h>
#include <utils/random.h>
#include <utils/sm3.h>
#include <utils/strings.h>
#include "private_key.h"
#include<main/configure.h>
#include "general.h"

#define PRIVATE_KEY_PREFIX  "pri"
#define PUBLIC_KEY_PREFIX  0xb0

namespace agent {

	utils::Mutex PrivateKeyBase::lock_;

	std::string KeypairUtils::EncodeAddress(SignatureType stype, EncodeType etype, const std::string& address) {
		std::string temp1;
		temp1.push_back(stype);
		temp1.push_back(etype);
		temp1.append(utils::Base58::Encode(address));
		return AddressPrefix::Instance().Encode(temp1);
	}
	std::string KeypairUtils::DecodeAddress(const std::string& address){
		if (address.size() < 2) return "";
		return utils::Base58::Decode(AddressPrefix::Instance().Decode(address).substr(2));
	}


	std::string KeypairUtils::GetAddressIndex(const std::string& address) {
		std::string str_result = "";
		std::string tmp = AddressPrefix::Instance().Decode(address);
		if (tmp.size() < 2){
            str_result = "";
        } 
		else {
             str_result = tmp.substr(0, 2) + utils::Base58::Decode(tmp.substr(2));
        }
		return str_result;
	}

	std::string KeypairUtils::EncodePublicKey(const std::string& key){
		return utils::String::BinToHexString(key);
	}

	std::string KeypairUtils::DecodePublicKey(const std::string& key){
		return utils::String::HexStringToBin(key);
	}
	std::string KeypairUtils::EncodePrivateKey(const std::string& key){
		return utils::Base58::Encode(key);
	}
	std::string KeypairUtils::DecodePrivateKey(const std::string& key){
		return utils::Base58::Decode(key);
	}

	std::string KeypairUtils::CalcHash(const std::string &value, const SignatureType &sign_type) {
		std::string hash;
		if (sign_type == SIGNTYPE_CFCASM2) {
			hash = utils::Sm3::Crypto(value);
		}
		else {
			hash = utils::Sha256::Crypto(value);
		}
		return hash;
	}

	bool KeypairUtils::GetPublicKeyElement(const std::string &encode_pub_key, KeyType &key_type, SignatureType &sign_type, EncodeType &encode_type, std::string &raw_data) {
		std::string buff = DecodePublicKey(encode_pub_key);
		if (buff.size() < 3)
			return false;

		uint8_t a = (uint8_t)buff.at(0);
		uint8_t b = (uint8_t)buff.at(1);
		uint8_t c = (uint8_t)buff.at(2);
		
		if (a != 0xb0)
			return false;

		sign_type = (SignatureType)b;
		encode_type = (EncodeType)c;
		size_t datalen = buff.size() - 3;

		bool ret = true;
		switch (sign_type) {
		case SIGNTYPE_ED25519:{
			ret = (ED25519_PUBLICKEY_LENGTH == datalen);
			break;
		}
		case SIGNTYPE_CFCASM2:{
			ret = (SM2_PUBLICKEY_LENGTH == datalen);
			break;
		}
		default:
			ret = false;
		}

		ret = (encode_type == ENCODETYPE_BASE58);
		//only support BASE 58
		
		if (ret) {
			key_type = KEY_PUBLICKEY;
			raw_data = buff.substr(3);
		}

		return ret;
	}

	bool KeypairUtils::GetKeyElement(const std::string &encode_key, KeyType &key_type_tmp, SignatureType &sign_type, EncodeType &encode_type, std::string &raw_data) {
		//SignatureType sign_type_tmp = SIGNTYPE_NONE;
		//EncodeType encode_type_tmp = ENCODETYPE_NONE;

		if (encode_key.find(AddressPrefix::Instance().address_prefix()) == 0) {// Address
			std::string base58addr = AddressPrefix::Instance().Decode(encode_key);

			std::string buff = DecodeAddress(base58addr);
			if (buff.size() != ED25519_ADDRESS_LENGTH) {
				return false;
			} 
			raw_data = buff.substr(2);
			key_type_tmp = KEY_ADDRESS;

			uint8_t a = (uint8_t)base58addr.at(0);
			uint8_t b = (uint8_t)base58addr.at(1);
			sign_type = (SignatureType)a;
			encode_type = (EncodeType)b;
			size_t datalen = buff.size();
			bool ret = false;
			switch (sign_type) {
			case SIGNTYPE_ED25519:{
				ret = (ED25519_ADDRESS_LENGTH == datalen);
				break;
			}
			case SIGNTYPE_CFCASM2:{
				ret = (SM2_ADDRESS_LENGTH == datalen);
				break;
			}
			default:
				ret = false;
			}

			ret = (encode_type == ENCODETYPE_BASE58); //only support BASE 58

			return ret;
		}
		else if (encode_key.find(PRIVATE_KEY_PREFIX) == 0) {//private key
			std::string buff = utils::Base58::Decode(encode_key);
			if (buff.size() != 37) {
				return false;
			}
			raw_data = buff.substr(5);
			key_type_tmp = KEY_PRIVATEKEY;

			uint8_t a = (uint8_t)buff.at(3);
			uint8_t b = (uint8_t)buff.at(4);
			sign_type = (SignatureType)a;
			encode_type = (EncodeType)b;
			size_t datalen = buff.size() - 5; //prefix length 3 + type 1 + encode type 1
			bool ret = false;
			switch (sign_type) {
			case SIGNTYPE_ED25519:{
				ret = (ED25519_PRIVATEKEY_LENGTH == datalen);
				break;
			}
			case SIGNTYPE_CFCASM2:{
				ret = (SM2_PRIVATEKEY_LENGTH == datalen);
				break;
			}
			default:
				ret = false;
			}

			ret = (encode_type == ENCODETYPE_BASE58); //only support BASE 58

			return ret;
		}

		return false;
	}

	std::string KeypairUtils::GetSignTypeDesc(SignatureType type) {
		switch (type) {
		case SIGNTYPE_CFCA: return "cfca";
		case SIGNTYPE_CFCASM2: return "sm2";
		case SIGNTYPE_RSA: return "rsa";
		case SIGNTYPE_ED25519: return "ed25519";
		}

		return "";
	}

	SignatureType KeypairUtils::GetSignTypeByDesc(const std::string &desc) {
        if (desc == "sm2") {
			return SIGNTYPE_CFCASM2;
		}
		else if (desc == "rsa") {
			return SIGNTYPE_RSA;
		}
		else if (desc == "ed25519") {
			return SIGNTYPE_ED25519;
		}
		return SIGNTYPE_NONE;
	}

	PublicKeyV4::PublicKeyV4() : PublicKeyBase(false, SIGNTYPE_ED25519){}

	PublicKeyV4::PublicKeyV4(bool valid, SignatureType type) : PublicKeyBase(valid, type){}

	PublicKeyV4::PublicKeyV4(const std::string &encode_pub_key) : PublicKeyBase(false, SIGNTYPE_ED25519){
		KeyType key_type;
		if (KeypairUtils::GetPublicKeyElement(encode_pub_key, key_type, type_, encode_type_, raw_pub_key_)) {
			valid_ = (key_type == KEY_PUBLICKEY);
		}
		std::string hash = KeypairUtils::CalcHash(raw_pub_key_, type_);
		raw_address_ = hash.substr(10);
	}

	void PublicKeyV4::Init(SignatureType type, std::string rawpkey) {
		type_ = type;
		raw_pub_key_ = rawpkey;
		std::string hash = KeypairUtils::CalcHash(raw_pub_key_, type_);
		raw_address_ = hash.substr(10);
	}


	std::map<std::string, bool> g_address_valid;
	utils::ReadWriteLock g_lock_address_valid;

	utils::StringMap g_pub_to_address;
	utils::ReadWriteLock g_lock_pub_to_address;
	std::string PublicKeyV4::GetEncAddress() const {
		std::string str_result = "";
		bool need_write = false;
		do {
			utils::ReadLockGuard guard(g_lock_pub_to_address);

			utils::StringMap::iterator iter = g_pub_to_address.find(raw_pub_key_);
			if (iter == g_pub_to_address.end()) {

				//Append public key 20byte
				std::string hash = KeypairUtils::CalcHash(raw_pub_key_, type_);
				str_result.append(hash.substr(10));

				str_result = KeypairUtils::EncodeAddress(type_, ENCODETYPE_BASE58, str_result);
				need_write = true;
			}
			else {
				str_result = iter->second;
			}

		} while (false);
		
		if (need_write){
			utils::WriteLockGuard guard(g_lock_pub_to_address);
			g_pub_to_address[raw_pub_key_] = str_result;
		} 

		return str_result;
	}

	std::string PublicKeyV4::GetEncPublicKey() const {
		
		std::string str_result = "";
		//Append PrivateKeyPrefix
		str_result.push_back((char)PUBLIC_KEY_PREFIX);

		//Append version
		str_result.push_back((char)type_);
		str_result.push_back((char)ENCODETYPE_BASE58);

		//Append public key
		str_result.append(raw_pub_key_);

		//printf("public key:%s\n", utils::String::BinToHexString(raw_pub_key_).c_str());
		return KeypairUtils::EncodePublicKey(str_result);
	}
	//not modify
	bool PublicKeyV4::Verify(const std::string &data, const std::string &signature, const std::string &encode_public_key) {
		KeyType key_type;
		SignatureType sign_type;
		EncodeType encode_type;
		std::string raw_pubkey;
		bool valid = KeypairUtils::GetPublicKeyElement(encode_public_key, key_type, sign_type, encode_type, raw_pubkey);
		if (!valid || key_type != KEY_PUBLICKEY) {
			return false;
		}

		if (signature.size() != 64) { 
			return false; 
		}

		if (sign_type == SIGNTYPE_ED25519 ) {
			return ed25519_sign_open((unsigned char *)data.c_str(), data.size(), (unsigned char *)raw_pubkey.c_str(), (unsigned char *)signature.c_str()) == 0;
		}
		
		if (sign_type == SIGNTYPE_CFCASM2) {
			return utils::EccSm2::verify(utils::EccSm2::GetCFCAGroup(), raw_pubkey, "1234567812345678", data, signature) == 1;
		}
		
		LOG_ERROR("Failed to verify. Unknown signature type(%d)", sign_type);
		return false;
	}

	//Generate keypair according to signature type.
	PrivateKeyV4::PrivateKeyV4(SignatureType type) {
		std::string raw_pub_key = "";
		type_ = type;
		if (type_ == SIGNTYPE_ED25519) {
			utils::MutexGuard guard_(lock_);
			// ed25519;
			raw_priv_key_.resize(32);
			//ed25519_randombytes_unsafe((void*)raw_priv_key_.c_str(), 32);
			if (!utils::GetStrongRandBytes(raw_priv_key_)){
				valid_ = false;
				return;
			}
			raw_pub_key.resize(32);
			ed25519_publickey((const unsigned char*)raw_priv_key_.c_str(), (unsigned char*)raw_pub_key.c_str());
		}
		else if (type_ == SIGNTYPE_CFCASM2) {
			utils::EccSm2 key(utils::EccSm2::GetCFCAGroup());
			key.NewRandom();
			raw_priv_key_ = key.getSkeyBin();
			raw_pub_key = key.GetPublicKey();
		}
		else{
			LOG_ERROR("Failed to verify.Unknown signature type(%d)", type_);
		}
		pub_key_ = std::make_shared<PublicKeyV4>(true, type_);
		pub_key_->Init(type_, raw_pub_key);
		valid_ = true;
	}

	bool PrivateKeyV4::From(const std::string &encode_private_key) {
		valid_ = false;
		std::string raw_pub;

		KeyType key_type;
		std::string raw_pubkey;
		valid_ = KeypairUtils::GetKeyElement(encode_private_key, key_type, type_, encode_type_, raw_priv_key_);
		if (!valid_) {
			return false;
		}

		if (key_type != KEY_PRIVATEKEY) {
			valid_ = false;
			return false;
		}

		if (type_ == SIGNTYPE_ED25519) {
			raw_pub.resize(32);
			ed25519_publickey((const unsigned char*)raw_priv_key_.c_str(), (unsigned char*)raw_pub.c_str());
		}
		else if (type_ == SIGNTYPE_CFCASM2) {
			utils::EccSm2 skey(utils::EccSm2::GetCFCAGroup());
			skey.From(raw_priv_key_);
			raw_pub = skey.GetPublicKey();
		}
		else{
			LOG_ERROR("Failed to verify.Unknown signature type(%d)", type_);
		}

		pub_key_ = std::make_shared<PublicKeyV4>(true, type_);
		pub_key_->Init(type_, raw_pub);
		valid_ = true;

		return valid_;
	}

	PrivateKeyV4::PrivateKeyV4(const std::string &encode_private_key) {
		From(encode_private_key );
	}

	void PrivateKeyV4::Init(SignatureType type, std::string raw_priv_key) {
		type_ = type;
		raw_priv_key_ = raw_priv_key;

		std::string raw_pub;
		if (type_ == SIGNTYPE_ED25519) {
			raw_pub.resize(32);
			ed25519_publickey((const unsigned char*)raw_priv_key_.c_str(), (unsigned char*)raw_pub.c_str());
		}
		else if (type_ == SIGNTYPE_CFCASM2) {
			utils::EccSm2 skey(utils::EccSm2::GetCFCAGroup());
			skey.From(raw_priv_key_);
			raw_pub = skey.GetPublicKey();
		}
		else{
			LOG_ERROR("Failed to verify.Unknown signature type(%d)", type_);
		}

		pub_key_ = std::make_shared<PublicKeyV4>(true, type_);
		pub_key_->Init(type_, raw_pub);
	}
	
	//not modify
	std::string PrivateKeyV4::Sign(const std::string &input) const {
		unsigned char sig[10240];
		unsigned int sig_len = 0;

		if (type_ == SIGNTYPE_ED25519) {
			/*	ed25519_signature sig;*/
			ed25519_sign((unsigned char *)input.c_str(), input.size(), (const unsigned char*)raw_priv_key_.c_str(), (unsigned char*)pub_key_->GetRawPublicKey().c_str(), sig);
			sig_len = 64;
		}
		else if (type_ == SIGNTYPE_CFCASM2) {
			utils::EccSm2 key(utils::EccSm2::GetCFCAGroup());
			key.From(raw_priv_key_);
			std::string r, s;
			return key.Sign("1234567812345678", input);
		}
		else{
			LOG_ERROR("Failed to verify.Unknown signature type(%d)", type_);
		}
		std::string output;
		output.append((const char *)sig, sig_len);
		return output;
	}

	std::string PrivateKeyV4::GetEncPrivateKey() const {
		std::string str_result;
		//Append prefix(pri) 0x18  0x9E  0X99
		str_result.push_back((char)24);
		str_result.push_back((char)158);
		str_result.push_back((char)153);

		//Append version 1
		str_result.push_back((char)type_);
		str_result.push_back((char)ENCODETYPE_BASE58);

		//Append private key 32
		str_result.append(raw_priv_key_);
		//printf("private key:%s\n", utils::String::BinToHexString(raw_priv_key_).c_str());

		return KeypairUtils::EncodePrivateKey(str_result);
	}

	PublicKey::~PublicKey() {}

	PublicKey::PublicKey() {
		pub_base_ = std::make_shared<PublicKeyV4>(); 
	}

	PublicKey::PublicKey(const std::string &encode_pub_key) {
		pub_base_ = std::make_shared<PublicKeyV4>(encode_pub_key);
	}

	void PublicKey::Init(SignatureType type, std::string rawpkey) {
		pub_base_->Init(type, rawpkey);
	}

	std::string PublicKey::GetEncAddress() const {
		return pub_base_->GetEncAddress();
	}

	std::string PublicKey::GetRawAddress() const {
		return pub_base_->GetRawAddress();
	}

	std::string PublicKey::GetEncPublicKey() const {
		return pub_base_->GetEncPublicKey();
	}

	std::string PublicKey::GetRawPublicKey() const {
		return pub_base_->GetRawPublicKey();
	}

	bool PublicKey::IsValid() const {
		return pub_base_->IsValid();
	}

	SignatureType PublicKey::GetSignType() const{
		return pub_base_->GetSignType();
	}

	bool PublicKey::Verify(const std::string &data, const std::string &signature, const std::string &encode_public_key) {
		 return PublicKeyV4::Verify(data, signature, encode_public_key);
	}

	utils::StringMap g_raw_address_to_encode;
	utils::ReadWriteLock g_lock_raw_address_to_encode;
	std::string PublicKey::EncodeAddress(const std::string &prefix, const std::string &raw_address) {
		if (prefix != "ef" && prefix != "zf") {
			return "";
		}

		do {
			utils::ReadLockGuard guard(g_lock_raw_address_to_encode);
			utils::StringMap::iterator iter = g_raw_address_to_encode.find(prefix + raw_address);
			if (iter != g_raw_address_to_encode.end()){
				return iter->second;
			} 
		} while (false);

		std::string encoded;
		if (prefix == "ef") {
			encoded = KeypairUtils::EncodeAddress(SIGNTYPE_ED25519, ENCODETYPE_BASE58, raw_address);
		}
		else {
			encoded = KeypairUtils::EncodeAddress(SIGNTYPE_CFCASM2, ENCODETYPE_BASE58, raw_address);
		}

		do {
			utils::WriteLockGuard guard(g_lock_raw_address_to_encode);
			g_raw_address_to_encode[prefix + raw_address] = encoded;
		} while (false);

		return encoded;
	}

	PrivateKey::~PrivateKey() {}

	PrivateKey::PrivateKey(SignatureType type) {
		priv_base_ = std::make_shared<PrivateKeyV4>(type);
	}

	PrivateKey::PrivateKey(const std::string &encode_private_key) {
		priv_base_ = std::make_shared<PrivateKeyV4>(encode_private_key);
	}

	bool PrivateKey::From(const std::string &encode_private_key) {
		return priv_base_->From(encode_private_key);
	}

	void PrivateKey::Init(SignatureType type, std::string raw_priv_key) {
		priv_base_->Init(type, raw_priv_key);
	}

	std::string	PrivateKey::Sign(const std::string &input) const {
		return priv_base_->Sign(input);
	}

	std::string PrivateKey::GetEncPrivateKey() const {
		return priv_base_->GetEncPrivateKey();
	}

	std::string PrivateKey::GetEncAddress() const {
		return priv_base_->GetEncAddress();
	}

	std::string PrivateKey::GetRawAddress() const {
		return priv_base_->GetRawAddress();
	}

	std::string PrivateKey::GetEncPublicKey() const {
		return priv_base_->GetEncPublicKey();
	}

	std::string PrivateKey::GetRawPublicKey() const {
		return priv_base_->GetRawPublicKey();
	}

	std::string PrivateKey::GetRawHexPrivateKey() const {
		return priv_base_->GetRawHexPrivateKey();
	}

	bool PrivateKey::IsValid() const {
		return priv_base_->IsValid();
	}

	std::string PrivateKey::GetRawPrivateKey() const{
		return priv_base_->GetRawPrivateKey();
	}

	SignatureType PrivateKey::GetSignType() const{
		return priv_base_->GetSignType();
	}

    static int32_t ledger_type_ = HashWrapper::HASH_TYPE_SHA256;
	HashWrapper::HashWrapper(){
		type_ = ledger_type_;
		if (type_ == HASH_TYPE_SM3){
			hash_ = new utils::Sm3();
		}

		else{
			hash_ = new utils::Sha256();
		}
	}

	HashWrapper::HashWrapper(int32_t type){
		type_ = type;
		if (type_ == HASH_TYPE_SM3){
			hash_ = new utils::Sm3();
		}
		else{
			hash_ = new utils::Sha256();
		}
	}

	HashWrapper::~HashWrapper(){
		if (hash_){
			delete hash_;
		} 
	}

	void HashWrapper::Update(const std::string &input){
		hash_->Update(input);
	}

	void HashWrapper::Update(const void *buffer, size_t len){
		hash_->Update(buffer, len);
	}

	std::string HashWrapper::Final(){
		return hash_->Final();
	}

	void HashWrapper::SetLedgerHashType(int32_t type_){
		ledger_type_ = type_;
	}

	int32_t HashWrapper::GetLedgerHashType(){
		return ledger_type_;
	}

	std::string HashWrapper::Crypto(const std::string &input){
		if (ledger_type_ == HASH_TYPE_SM3){
			return utils::Sm3::Crypto(input);
		}
		else{
			return utils::Sha256::Crypto(input);
		}
	}

	void HashWrapper::Crypto(unsigned char* str, int len, unsigned char *buf){
		if (ledger_type_ == HASH_TYPE_SM3){
			utils::Sm3::Crypto(str, len, buf);
		}
		else{
			utils::Sha256::Crypto(str, len, buf);
		}
	}

	void HashWrapper::Crypto(const std::string &input, std::string &str){
		if (ledger_type_ == HASH_TYPE_SM3){
			utils::Sm3::Crypto(input, str);
		}
		else{
			utils::Sha256::Crypto(input, str);
		}
	}
}
