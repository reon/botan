/*
* BER Decoder
* (C) 1999-2010 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#ifndef BOTAN_BER_DECODER_H__
#define BOTAN_BER_DECODER_H__

#include <botan/asn1_oid.h>
#include <botan/data_src.h>

namespace Botan {

/**
* BER Decoding Object
*/
class BOTAN_DLL BER_Decoder
   {
   public:
      BER_Object get_next_object();
      void push_back(const BER_Object& obj);

      bool more_items() const;
      BER_Decoder& verify_end();
      BER_Decoder& discard_remaining();

      BER_Decoder  start_cons(ASN1_Tag type_tag, ASN1_Tag class_tag = UNIVERSAL);
      BER_Decoder& end_cons();

      BER_Decoder& raw_bytes(secure_vector<byte>& v);
      BER_Decoder& raw_bytes(std::vector<byte>& v);

      BER_Decoder& decode_null();
      BER_Decoder& decode(bool& v);
      BER_Decoder& decode(size_t& v);
      BER_Decoder& decode(class BigInt& v);
      BER_Decoder& decode(std::vector<byte>& v, ASN1_Tag type_tag);
      BER_Decoder& decode(secure_vector<byte>& v, ASN1_Tag type_tag);

      BER_Decoder& decode(bool& v,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      BER_Decoder& decode(size_t& v,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      BER_Decoder& decode(class BigInt& v,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      BER_Decoder& decode(std::vector<byte>& v,
                          ASN1_Tag real_type,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      BER_Decoder& decode(secure_vector<byte>& v,
                          ASN1_Tag real_type,
                          ASN1_Tag type_tag,
                          ASN1_Tag class_tag = CONTEXT_SPECIFIC);

      BER_Decoder& decode(class ASN1_Object& obj);

      BER_Decoder& decode_octet_string_bigint(class BigInt& b);

      u64bit decode_constrained_integer(ASN1_Tag type_tag,
                                        ASN1_Tag class_tag,
                                        size_t T_bytes);

      template<typename T> BER_Decoder& decode_integer_type(T& out)
         {
         return decode_integer_type<T>(out, INTEGER, UNIVERSAL);
         }

      template<typename T>
         BER_Decoder& decode_integer_type(T& out,
                                          ASN1_Tag type_tag,
                                          ASN1_Tag class_tag = CONTEXT_SPECIFIC)
         {
         out = decode_constrained_integer(type_tag, class_tag, sizeof(out));
         return (*this);
         }

      template<typename T>
         BER_Decoder& decode_optional(T& out,
                                      ASN1_Tag type_tag,
                                      ASN1_Tag class_tag,
                                      const T& default_value = T());

      template<typename T>
         BER_Decoder& decode_list(std::vector<T>& out,
                                  bool clear_out = true);

      template<typename T>
         BER_Decoder& decode_and_check(const T& expected,
                                       const std::string& error_msg)
         {
         T actual;
         decode(actual);

         if(actual != expected)
            throw Decoding_Error(error_msg);

         return (*this);
         }

      BER_Decoder& decode_optional_string(std::vector<byte>& out,
                                          ASN1_Tag real_type,
                                          u16bit type_no);

      BER_Decoder& decode_optional_string(secure_vector<byte>& out,
                                          ASN1_Tag real_type,
                                          u16bit type_no);

      BER_Decoder& operator=(const BER_Decoder&) = delete;

      BER_Decoder(DataSource&);

      BER_Decoder(const byte[], size_t);

      BER_Decoder(const secure_vector<byte>&);

      BER_Decoder(const std::vector<byte>& vec);

      BER_Decoder(const BER_Decoder&);
      ~BER_Decoder();
   private:
      BER_Decoder* parent;
      DataSource* source;
      BER_Object pushed;
      mutable bool owns;
   };

/*
* Decode an OPTIONAL or DEFAULT element
*/
template<typename T>
BER_Decoder& BER_Decoder::decode_optional(T& out,
                                          ASN1_Tag type_tag,
                                          ASN1_Tag class_tag,
                                          const T& default_value)
   {
   BER_Object obj = get_next_object();

   if(obj.type_tag == type_tag && obj.class_tag == class_tag)
      {
      if(class_tag & CONSTRUCTED)
         BER_Decoder(obj.value).decode(out).verify_end();
      else
         {
         push_back(obj);
         decode(out, type_tag, class_tag);
         }
      }
   else
      {
      out = default_value;
      push_back(obj);
      }

   return (*this);
   }

/*
* Decode a list of homogenously typed values
*/
template<typename T>
BER_Decoder& BER_Decoder::decode_list(std::vector<T>& vec, bool clear_it)
   {
   if(clear_it)
      vec.clear();

   while(more_items())
      {
      T value;
      decode(value);
      vec.push_back(value);
      }
   return (*this);
   }

}

#endif
