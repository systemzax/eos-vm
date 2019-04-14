#pragma once

#include <iostream>
#include <cstdlib>
#include <eosio/wasm_backend/exceptions.hpp>
#include <eosio/wasm_backend/utils.hpp>

namespace eosio { namespace wasm_backend {

   template <size_t N, typename T>
   struct shift_amount {
      static constexpr size_t value = ((sizeof(type) * 8) - 1) - N;
   };
   
   template <size_t N, typename T, typename R>
   constexpr R sign_extend(R res) {
      return (static_cast<T>((res) << shift_amount<N, T>::value >> shift_amount<N, T>::value;
   }

   template <size_t N> 
   constexpr size_t zero_extended_size() {
      if constexpr ( N == 1 || N == 7 )
         return 1;
      else if ( N == 32 )
         return 5;
      else
         return 9;
   } 
   
   template <size_t N, size_t S>
   constexpr void sign_extend( uint8_t (&raw)[S] ) {
      raw[N] |= 0x80;
   }
   
   template <size_t N>
   struct varuint {
      uint8_t raw[zero_extended_size<N>()] = {0};

      uint8_t size = 0;
      varuint() = default;
      varuint(uint64_t n) {
         set(n);
      }
      varuint( const std::vector<uint8_t>& code, size_t index ) {
         set(code, index);
      }
      varuint( const varuint<N>& n ) {
         memcpy(raw, n.raw, n.size);
         size = n.size;
      }
      varuint& operator=( const varuint& n ) {
         memcpy(raw, n.raw, n.size);
         size = n.size;
         return *this;
      }

      inline void set( const std::vector<uint8_t>& code, size_t index ) {
         uint8_t cnt = 0;
         for (; cnt < zero_extended_size<N>(); cnt++ ) {
            EOS_WB_ASSERT( index+cnt < code.size(), wasm_interpreter_exception, "varuint not terminated before end of code" );
            raw[cnt] = code[index+cnt];
            if ((raw[cnt] & 0x80) == 0)
               break;
         }
         size = cnt+1;
      }

      inline void set( guarded_ptr<uint8_t>& code ) {
         for (uint8_t cnt=0; cnt < zero_extended_size<N>(); cnt++ ) {
            EOS_WB_ASSERT( code.offset()+cnt < code.bounds(), wasm_interpreter_exception, "pointer out of bounds" );
            raw[cnt] = code[cnt];
            if ((raw[cnt] & 0x80) == 0) {
               size = cnt+1;
               code += size;
               break;
            }
         }
      }

      inline void set(unsigned __int128 n) {
         uint8_t cnt = 1;
         guarded_ptr<uint8_t> data( raw, zero_extended_size<N>() );
         EOS_WB_ASSERT( n < ((unsigned __int128)1 << N), wasm_interpreter_exception, 
               "value too large for bit width specified" );
         for (; cnt < sizeof(n); cnt++) {
            uint8_t byte = n & 0x7F;
            n >>= 7;
            if (n != 0)
               byte |= 0x80;
            *data++ = byte;
            if (n == 0)
               break;
         }
         size = cnt; 
      }

      inline uint64_t get() {
         uint64_t n = 0;
         uint8_t shift = 0;
         const uint8_t* end = raw + size;
         uint8_t* data = raw;
         bool quit = false;
         for (int i=0; i < size; i++) {
            EOS_WB_ASSERT( end && data != end, wasm_interpreter_exception, "malformed varuint");
            uint64_t byte = *data & 0x7F;
            EOS_WB_ASSERT( !(shift >= 64 || byte << shift >> shift != byte), 
                  wasm_interpreter_exception, "varuint too big for uint64");
            n += byte << shift;
            shift += 7;
            if ( quit )
               break;
            if ( *data++ <= 0x80 )
               quit = true;

         };
         return n;
      }
   };

   template <size_t N>
   struct varint {
      uint8_t raw[zero_extended_size<N>()];
      uint8_t size;
      varint() = default;
      varint(int64_t n) {
         set(n);
      }
      varint( const std::vector<uint8_t>& code, size_t index ) {
         set(code, index);
      }
      varint( const varint<N>& n ) {
         memcpy(raw, n.raw, n.size);
         size = n.size;
      }
      varint& operator=( const varint<N>& n ) {
         memcpy(raw, n.raw, n.size);
         size = n.size;
         return *this;
      }

      inline void set( const std::vector<uint8_t>& code, size_t index ) {
         uint8_t cnt = 0;
         if ((*code++ & 0x80) == 0) {

         }
         for (; cnt < zero_extended_size<N>(); cnt++ ) {
            EOS_WB_ASSERT( index+cnt < code.size(), wasm_interpreter_exception, "varuint not terminated before end of code" );
            raw[cnt] = code[index+cnt];
            if ((raw[cnt] & 0x7F) == 0)
               break;
         }
         size = cnt+1;
      }
      #define BYTE_AT(type, i, shift) ((static_cast<type>(code[i] & 0x7f) << (shift))

#define LEB128_1(type) (BYTE_AT(type, 0, 0))
#define LEB128_2(type) (BYTE_AT(type, 1, 7) | LEB128_1(type))
#define LEB128_3(type) (BYTE_AT(type, 2, 14) | LEB128_2(type))
#define LEB128_4(type) (BYTE_AT(type, 3, 21) | LEB128_3(type))
#define LEB128_5(type) (BYTE_AT(type, 4, 28) | LEB128_4(type))
#define LEB128_6(type) (BYTE_AT(type, 5, 35) | LEB128_5(type))
#define LEB128_7(type) (BYTE_AT(type, 6, 42) | LEB128_6(type))
#define LEB128_8(type) (BYTE_AT(type, 7, 49) | LEB128_7(type))
#define LEB128_9(type) (BYTE_AT(type, 8, 56) | LEB128_8(type))
#define LEB128_10(type) (BYTE_AT(type, 9, 63) | LEB128_9(type))

#define SHIFT_AMOUNT(type, sign_bit) (sizeof(type) * 8 - 1 - (sign_bit))
#define SIGN_EXTEND(type, value, sign_bit)                       \
     (static_cast<type>((value) << SHIFT_AMOUNT(type, sign_bit)) >> \
      SHIFT_AMOUNT(type, sign_bit))

      int64_t gg( guarded_ptr<uint8_t>& code ) {
         if ((*code++ & 0x80) == 0) {
           uint64_t result = LEB128_1(uint64_t);
           *out_value = SIGN_EXTEND(int64_t, result, 6);
           return 1;
         } else if ((*code++ & 0x80) == 0) {
           uint64_t result = LEB128_2(uint64_t);
           *out_value = SIGN_EXTEND(int64_t, result, 13);
           return 2;
         } else if ((*code++ & 0x80) == 0) {
           uint64_t result = LEB128_3(uint64_t);
           *out_value = SIGN_EXTEND(int64_t, result, 20);
           return 3;
         } else if ((*code++ & 0x80) == 0) {
           uint64_t result = LEB128_4(uint64_t);
           *out_value = SIGN_EXTEND(int64_t, result, 27);
           return 4;
         } else if ((*code++ & 0x80) == 0) {
           uint64_t result = LEB128_5(uint64_t);
           *out_value = SIGN_EXTEND(int64_t, result, 34);
           return 5;
         } else if ((*code++ & 0x80) == 0) {
           uint64_t result = LEB128_6(uint64_t);
           *out_value = SIGN_EXTEND(int64_t, result, 41);
           return 6;
         } else if ((*code++ & 0x80) == 0) {
           uint64_t result = LEB128_7(uint64_t);
           *out_value = SIGN_EXTEND(int64_t, result, 48);
           return 7;
         } else if ((*code++ & 0x80) == 0) {
           uint64_t result = LEB128_8(uint64_t);
           *out_value = SIGN_EXTEND(int64_t, result, 55);
           return 8;
         } else if ((*code++ & 0x80) == 0) {
           uint64_t result = LEB128_9(uint64_t);
           *out_value = SIGN_EXTEND(int64_t, result, 62);
           return 9;
         } else if ((*code++ & 0x80) == 0) {
           // The top bits should be a sign-extension of the sign bit.
           bool sign_bit_set = (p[9] & 0x1);
           int top_bits = p[9] & 0xfe;
           if ((sign_bit_set && top_bits != 0x7e) ||
               (!sign_bit_set && top_bits != 0)) {
             return 0;
           }
           uint64_t result = LEB128_10(uint64_t);
           *out_value = result;
           return 10;
         } else {
           // Past the end.
           return 0;
         }
      }

      inline void set( guarded_ptr<uint8_t>& code ) {
         for (uint8_t cnt=0; cnt < zero_extended_size<N>(); cnt++ ) {
            EOS_WB_ASSERT( code.offset()+cnt < code.bounds(), wasm_interpreter_exception, "pointer out of bounds" );
            raw[cnt] = code[cnt];
            std::cout << std::hex << (uint32_t)raw[cnt] << " ";
            if ((raw[cnt] & 0x80) == 0 || (raw[cnt] & 0xFF) == 0) {
               size = cnt+1;
               code += size;
               break;
            }
         }
         std::cout << std::dec << "\n";
      }

      inline void set(__int128 n) {
         uint8_t* data = raw;
         uint8_t cnt = 1;
         bool more = true;
         for (; more && cnt < sizeof(n); cnt++) {
            uint8_t byte = n & 0x7F;
            n >>= 7;
            more = !((((n == 0) && ((byte & 0x40) == 0)) ||
                     ((n == -1) && ((byte & 0x40) != 0))));

            if (more)
               byte |= 0x80;
            *data++ = byte;
            if (!more)
               break;
         }
         size = cnt;
      }

      inline int64_t get() const {
         int64_t n = 0;
         uint8_t shift = 0;
         uint8_t byte = 0;
         const uint8_t* end = raw + size;
         const uint8_t* data = raw;
         
         for (int i=0; i < size; i++) {
            EOS_WB_ASSERT( end && data != end, wasm_interpreter_exception, "malformed varint");
            byte = *data++;
            n |= (int64_t)(byte & 0x7F) << shift;
            shift += 7;
            if ( byte < 0x80 )
               break;
         };

         if (byte & 0x40)
            n |= (-1ull) << shift;
         return n;
      }
   };

   /*
   void str_bits(varuint<32> n) {
      for (int i=32; i >= 0; i--) {
         if (i % 8 == 0)
            std::cout << " ";
         std::cout << (uint32_t)((n.raw[i/8] >> i%8) & 1) << " ";
      }
      std::cout << "\n";
   }
   */

}} // namespace eosio::wasm_backend