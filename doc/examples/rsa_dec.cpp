/*
* (C) 2002-2010 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

/*
Decrypt an encrypted RSA private key. Then use that key to decrypt a
message. This program can decrypt messages generated by rsa_enc, and uses the
same key format as that generated by rsa_kgen.
*/

#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include <botan/botan.h>
#include <botan/pubkey.h>
#include <botan/rsa.h>
using namespace Botan;

secure_vector<byte> b64_decode(const std::string&);
SymmetricKey derive_key(const std::string&, const SymmetricKey&, u32bit);

const std::string SUFFIX = ".enc";

int main(int argc, char* argv[])
   {
   if(argc != 4)
      {
      std::cout << "Usage: " << argv[0] << " keyfile messagefile passphrase"
                << std::endl;
      return 1;
      }

   Botan::LibraryInitializer init;

   try
      {
      AutoSeeded_RNG rng;

      std::auto_ptr<PKCS8_PrivateKey> key(
         PKCS8::load_key(std::string(argv[1]), rng, std::string(argv[3])));

      RSA_PrivateKey* rsakey = dynamic_cast<RSA_PrivateKey*>(key.get());
      if(!rsakey)
         {
         std::cout << "The loaded key is not a RSA key!\n";
         return 1;
         }

      std::ifstream message(argv[2]);
      if(!message)
         {
         std::cout << "Couldn't read the message file." << std::endl;
         return 1;
         }

      std::string outfile(argv[2]);
      outfile = outfile.replace(outfile.find(SUFFIX), SUFFIX.length(), "");

      std::ofstream plaintext(outfile.c_str(), std::ios::binary);
      if(!plaintext)
         {
         std::cout << "Couldn't write the plaintext to "
                   << outfile << std::endl;
         return 1;
         }

      std::string enc_masterkey_str;
      std::getline(message, enc_masterkey_str);
      std::string mac_str;
      std::getline(message, mac_str);

      secure_vector<byte> enc_masterkey = b64_decode(enc_masterkey_str);

      PK_Decryptor_EME decryptor(*rsakey, "EME1(SHA-1)");

      secure_vector<byte> masterkey = decryptor.decrypt(enc_masterkey);

      SymmetricKey cast_key   = derive_key("CAST", masterkey, 16);
      InitializationVector iv = derive_key("IV",   masterkey, 8);
      SymmetricKey mac_key    = derive_key("MAC",  masterkey, 16);

      Pipe pipe(new Base64_Decoder,
                get_cipher("CAST-128/CBC/PKCS7", cast_key, iv, DECRYPTION),
                new Fork(
                   0,
                   new Chain(
                      new MAC_Filter("HMAC(SHA-1)", mac_key, 12),
                      new Base64_Encoder
                      )
                   )
         );

      pipe.start_msg();
      message >> pipe;
      pipe.end_msg();

      std::string our_mac = pipe.read_all_as_string(1);

      if(our_mac != mac_str)
         std::cout << "WARNING: MAC in message failed to verify\n";

      plaintext << pipe.read_all_as_string(0);
   }
   catch(std::exception& e)
      {
      std::cout << "Exception caught: " << e.what() << std::endl;
      return 1;
      }
   return 0;
   }

secure_vector<byte> b64_decode(const std::string& in)
   {
   Pipe pipe(new Base64_Decoder);
   pipe.process_msg(in);
   return pipe.read_all();
   }

SymmetricKey derive_key(const std::string& param,
                        const SymmetricKey& masterkey,
                        u32bit outputlength)
   {
   std::auto_ptr<KDF> kdf(get_kdf("KDF2(SHA-1)"));
   return kdf->derive_key(outputlength, masterkey.bits_of(), param);
   }
