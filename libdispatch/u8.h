/*
	Copyright (C) 2014-2016 Quinten Lansu

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

/* This is the concatenation of

base.h casemapping.h codepoint.h database.h streaming.h
composition.h decomposition.h unicodedatabase.h utf8rewind.h
(order is important)

with some modifications to simplify
*/

#ifndef U8_H
#define U8_H 1

#ifndef _UTF8REWIND_H_
#define _UTF8REWIND_H_




#define UTF8_VERSION_MAKE(_major, _minor, _bugfix) \
	((_major) * 10000) + ((_minor) * 100) + (_bugfix)

#define UTF8_VERSION_MAJOR   1

#define UTF8_VERSION_MINOR   5

#define UTF8_VERSION_BUGFIX  1

#define UTF8_VERSION \
	UTF8_VERSION_MAKE(UTF8_VERSION_MAJOR, UTF8_VERSION_MINOR, UTF8_VERSION_BUGFIX)

#define UTF8_VERSION_STRING  "1.5.1"

#define UTF8_VERSION_GUARD(_major, _minor, _bugfix) \
	(UTF8_VERSION >= UTF8_VERSION_MAKE(_major, _minor, _bugfix))



#define UTF8_ERR_NONE                           (0)

#define UTF8_ERR_INVALID_DATA                   (-1)

#define UTF8_ERR_INVALID_FLAG                   (-2)

#define UTF8_ERR_NOT_ENOUGH_SPACE               (-3)

#define UTF8_ERR_OVERLAPPING_PARAMETERS         (-4)

#define UTF8_ERR_INVALID_LOCALE                 (-5)



#define UTF8_LOCALE_DEFAULT                     0

#define UTF8_LOCALE_LITHUANIAN                  1

#define UTF8_LOCALE_TURKISH_AND_AZERI_LATIN     2

#define UTF8_LOCALE_MAXIMUM                     3



#define UTF8_NORMALIZE_COMPOSE                  0x00000001

#define UTF8_NORMALIZE_DECOMPOSE                0x00000002

#define UTF8_NORMALIZE_COMPATIBILITY            0x00000004

#define UTF8_NORMALIZATION_RESULT_YES           (0)

#define UTF8_NORMALIZATION_RESULT_MAYBE         (1)

#define UTF8_NORMALIZATION_RESULT_NO            (2)



#define UTF8_CATEGORY_LETTER_UPPERCASE          0x00000001

#define UTF8_CATEGORY_LETTER_LOWERCASE          0x00000002

#define UTF8_CATEGORY_LETTER_TITLECASE          0x00000004

#define UTF8_CATEGORY_LETTER_MODIFIER           0x00000008

#define UTF8_CATEGORY_LETTER_OTHER              0x00000010

#define UTF8_CATEGORY_LETTER \
	(UTF8_CATEGORY_LETTER_UPPERCASE | UTF8_CATEGORY_LETTER_LOWERCASE | \
	UTF8_CATEGORY_LETTER_TITLECASE | UTF8_CATEGORY_LETTER_MODIFIER | \
	UTF8_CATEGORY_LETTER_OTHER)

#define UTF8_CATEGORY_CASE_MAPPED \
	(UTF8_CATEGORY_LETTER_UPPERCASE | UTF8_CATEGORY_LETTER_LOWERCASE | \
	UTF8_CATEGORY_LETTER_TITLECASE)

#define UTF8_CATEGORY_MARK_NON_SPACING          0x00000020

#define UTF8_CATEGORY_MARK_SPACING              0x00000040

#define UTF8_CATEGORY_MARK_ENCLOSING            0x00000080

#define UTF8_CATEGORY_MARK \
	(UTF8_CATEGORY_MARK_NON_SPACING | UTF8_CATEGORY_MARK_SPACING | \
	UTF8_CATEGORY_MARK_ENCLOSING)

#define UTF8_CATEGORY_NUMBER_DECIMAL            0x00000100

#define UTF8_CATEGORY_NUMBER_LETTER             0x00000200

#define UTF8_CATEGORY_NUMBER_OTHER              0x00000400

#define UTF8_CATEGORY_NUMBER \
	(UTF8_CATEGORY_NUMBER_DECIMAL | UTF8_CATEGORY_NUMBER_LETTER | \
	UTF8_CATEGORY_NUMBER_OTHER)

#define UTF8_CATEGORY_PUNCTUATION_CONNECTOR     0x00000800

#define UTF8_CATEGORY_PUNCTUATION_DASH          0x00001000

#define UTF8_CATEGORY_PUNCTUATION_OPEN          0x00002000

#define UTF8_CATEGORY_PUNCTUATION_CLOSE         0x00004000

#define UTF8_CATEGORY_PUNCTUATION_INITIAL       0x00008000

#define UTF8_CATEGORY_PUNCTUATION_FINAL         0x00010000

#define UTF8_CATEGORY_PUNCTUATION_OTHER         0x00020000

#define UTF8_CATEGORY_PUNCTUATION \
	(UTF8_CATEGORY_PUNCTUATION_CONNECTOR | UTF8_CATEGORY_PUNCTUATION_DASH | \
	UTF8_CATEGORY_PUNCTUATION_OPEN | UTF8_CATEGORY_PUNCTUATION_CLOSE | \
	UTF8_CATEGORY_PUNCTUATION_INITIAL | UTF8_CATEGORY_PUNCTUATION_FINAL | \
	UTF8_CATEGORY_PUNCTUATION_OTHER)

#define UTF8_CATEGORY_SYMBOL_MATH               0x00040000

#define UTF8_CATEGORY_SYMBOL_CURRENCY           0x00080000

#define UTF8_CATEGORY_SYMBOL_MODIFIER           0x00100000

#define UTF8_CATEGORY_SYMBOL_OTHER              0x00200000

#define UTF8_CATEGORY_SYMBOL \
	(UTF8_CATEGORY_SYMBOL_MATH | UTF8_CATEGORY_SYMBOL_CURRENCY | \
	UTF8_CATEGORY_SYMBOL_MODIFIER | UTF8_CATEGORY_SYMBOL_OTHER)

#define UTF8_CATEGORY_SEPARATOR_SPACE           0x00400000

#define UTF8_CATEGORY_SEPARATOR_LINE            0x00800000

#define UTF8_CATEGORY_SEPARATOR_PARAGRAPH       0x01000000

#define UTF8_CATEGORY_SEPARATOR \
	(UTF8_CATEGORY_SEPARATOR_SPACE | UTF8_CATEGORY_SEPARATOR_LINE | \
	UTF8_CATEGORY_SEPARATOR_PARAGRAPH)

#define UTF8_CATEGORY_CONTROL                   0x02000000

#define UTF8_CATEGORY_FORMAT                    0x04000000

#define UTF8_CATEGORY_SURROGATE                 0x08000000

#define UTF8_CATEGORY_PRIVATE_USE               0x10000000

#define UTF8_CATEGORY_UNASSIGNED                0x20000000

#define UTF8_CATEGORY_COMPATIBILITY             0x40000000

#define UTF8_CATEGORY_IGNORE_GRAPHEME_CLUSTER   0x80000000

#define UTF8_CATEGORY_ISCNTRL \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_CONTROL)

#define UTF8_CATEGORY_ISPRINT \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_LETTER | UTF8_CATEGORY_NUMBER | \
	UTF8_CATEGORY_PUNCTUATION | UTF8_CATEGORY_SYMBOL | \
	UTF8_CATEGORY_SEPARATOR)

#define UTF8_CATEGORY_ISSPACE \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_SEPARATOR_SPACE)

#define UTF8_CATEGORY_ISBLANK \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_SEPARATOR_SPACE | UTF8_CATEGORY_PRIVATE_USE)

#define UTF8_CATEGORY_ISGRAPH \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_LETTER | UTF8_CATEGORY_NUMBER | \
	UTF8_CATEGORY_PUNCTUATION | UTF8_CATEGORY_SYMBOL)

#define UTF8_CATEGORY_ISPUNCT \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_PUNCTUATION | UTF8_CATEGORY_SYMBOL)

#define UTF8_CATEGORY_ISALNUM \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_LETTER | UTF8_CATEGORY_NUMBER)

#define UTF8_CATEGORY_ISALPHA \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_LETTER)

#define UTF8_CATEGORY_ISUPPER \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_LETTER_UPPERCASE)

#define UTF8_CATEGORY_ISLOWER \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_LETTER_LOWERCASE)

#define UTF8_CATEGORY_ISDIGIT \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_NUMBER)

#define UTF8_CATEGORY_ISXDIGIT \
	(UTF8_CATEGORY_COMPATIBILITY | \
	UTF8_CATEGORY_NUMBER | UTF8_CATEGORY_PRIVATE_USE)




#ifndef UTF8_WCHAR_SIZE
	#if (__SIZEOF_WCHAR_T__ == 4) || (WCHAR_MAX > UINT16_MAX) || (__WCHAR_MAX__ > UINT16_MAX)
		#define UTF8_WCHAR_SIZE (4)
	#else
		#define UTF8_WCHAR_SIZE (2)
	#endif
#endif

#if (UTF8_WCHAR_SIZE == 4)
	
	#define UTF8_WCHAR_UTF32 (1)
#elif (UTF8_WCHAR_SIZE == 2)
	
	#define UTF8_WCHAR_UTF16 (1)
#else
	#error Invalid size for wchar_t type.
#endif

#ifndef UTF8_API
	#ifdef __cplusplus
		#define UTF8_API extern "C"
	#else
		#define UTF8_API
	#endif
#endif

typedef int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef uint16_t utf16_t;
typedef uint32_t unicode_t;

#endif /* _UTF8REWIND_H_ */


#ifndef _UTF8REWIND_INTERNAL_BASE_H_
#define _UTF8REWIND_INTERNAL_BASE_H_




#if defined(__GNUC__) && !defined(COMPILER_ICC)
	#define UTF8_UNUSED(_parameter) _parameter __attribute__ ((unused))
#else
	#define UTF8_UNUSED(_parameter) _parameter
#endif

#define UTF8_SET_ERROR(_error) \
	if (errors != 0) { *errors = UTF8_ERR_ ## _error; }

/* Validates input before transforming */
/* Check for parameter overlap using the separating axis theorem */

#define UTF8_VALIDATE_PARAMETERS_CHAR(_inputType, _result) \
	if (input == 0) { \
		UTF8_SET_ERROR(INVALID_DATA); \
		return _result; \
	} \
	else if (inputSize < sizeof(_inputType)) { \
		if (target != 0) { \
			if (targetSize < 3) { \
				UTF8_SET_ERROR(NOT_ENOUGH_SPACE); \
				return _result; \
			} \
			memcpy(target, REPLACEMENT_CHARACTER_STRING, REPLACEMENT_CHARACTER_STRING_LENGTH); \
		} \
		UTF8_SET_ERROR(INVALID_DATA); \
		return _result + REPLACEMENT_CHARACTER_STRING_LENGTH; \
	} \
	if (target != 0 && targetSize == 0) { \
		UTF8_SET_ERROR(NOT_ENOUGH_SPACE); \
		return _result; \
	} \
	if ((char*)input == target) { \
		UTF8_SET_ERROR(OVERLAPPING_PARAMETERS); \
		return _result; \
	} \
	{ \
		char* input_center = (char*)input + (inputSize / 2); \
		char* target_center = target + (targetSize / 2); \
		size_t delta = (size_t)((input_center > target_center) ? (input_center - target_center) : (target_center - input_center)); \
		if (delta < (inputSize + targetSize) / 2) { \
			UTF8_SET_ERROR(OVERLAPPING_PARAMETERS); \
			return _result; \
		} \
	}

#define UTF8_VALIDATE_PARAMETERS(_inputType, _outputType, _result) \
	if (input == 0) { \
		UTF8_SET_ERROR(INVALID_DATA); \
		return _result; \
	} \
	else if (inputSize < sizeof(_inputType)) { \
		if (target != 0) { \
			if (targetSize < sizeof(_outputType)) { \
				UTF8_SET_ERROR(NOT_ENOUGH_SPACE); \
				return _result; \
			} \
			*target = REPLACEMENT_CHARACTER; \
		} \
		UTF8_SET_ERROR(INVALID_DATA); \
		return _result + sizeof(_outputType); \
	} \
	if (target != 0 && targetSize < sizeof(_outputType)) { \
		UTF8_SET_ERROR(NOT_ENOUGH_SPACE); \
		return _result; \
	} \
	if ((char*)input == (char*)target) { \
		UTF8_SET_ERROR(OVERLAPPING_PARAMETERS); \
		return _result; \
	} \
	{ \
		char* input_center = (char*)input + (inputSize / 2); \
		char* target_center = (char*)target + (targetSize / 2); \
		size_t delta = (size_t)((input_center > target_center) ? (input_center - target_center) : (target_center - input_center)); \
		if (delta < (inputSize + targetSize) / 2) { \
			UTF8_SET_ERROR(OVERLAPPING_PARAMETERS); \
			return _result; \
		} \
	}



#endif /* _UTF8REWIND_INTERNAL_BASE_H_ */

#ifndef _UTF8REWIND_INTERNAL_CASEMAPPING_H_
#define _UTF8REWIND_INTERNAL_CASEMAPPING_H_




typedef struct {
	const char* src;
	char* dst;
	size_t src_size;
	size_t dst_size;
	size_t total_bytes_needed;
	unicode_t last_code_point;
	size_t locale;
	const uint32_t* property_index1;
	const uint32_t* property_index2;
	const uint32_t* property_data;
	uint32_t last_general_category;
	uint8_t last_code_point_size;
	uint8_t last_canonical_combining_class;
	uint8_t quickcheck_flags;
} CaseMappingState;

uint8_t casemapping_initialize(
	CaseMappingState* state,
	const char* input, size_t inputSize,
	char* target, size_t targetSize,
	const uint32_t* propertyIndex1, const uint32_t* propertyIndex2, const uint32_t* propertyData,
	uint8_t quickCheck, size_t locale,
	int32_t* errors);

size_t casemapping_execute(CaseMappingState* state, int32_t* errors);



#endif /* _UTF8REWIND_INTERNAL_CASEMAPPING_H_ */
#ifndef _UTF8REWIND_INTERNAL_CODEPOINT_H_
#define _UTF8REWIND_INTERNAL_CODEPOINT_H_







#define MAX_BASIC_LATIN                      0x007F


#define MAX_LATIN_1                          0x00FF


#define MAX_BASIC_MULTILINGUAL_PLANE         0xFFFF


#define MAX_LEGAL_UNICODE                    0x10FFFF


#define REPLACEMENT_CHARACTER                0xFFFD


#define REPLACEMENT_CHARACTER_STRING         "\xEF\xBF\xBD"


#define REPLACEMENT_CHARACTER_STRING_LENGTH  3


#define SURROGATE_HIGH_START                 0xD800


#define SURROGATE_HIGH_END                   0xDBFF


#define SURROGATE_LOW_START                  0xDC00


#define SURROGATE_LOW_END                    0xDFFF


#define HANGUL_JAMO_FIRST                    0x1100


#define HANGUL_JAMO_LAST                     0x11FF


#define HANGUL_L_FIRST                       0x1100


#define HANGUL_L_LAST                        0x1112


#define HANGUL_L_COUNT                       19


#define HANGUL_V_FIRST                       0x1161


#define HANGUL_V_LAST                        0x1175


#define HANGUL_V_COUNT                       21


#define HANGUL_T_FIRST                       0x11A7


#define HANGUL_T_LAST                        0x11C2


#define HANGUL_T_COUNT                       28


#define HANGUL_N_COUNT                       588 /* VCount * TCount */


#define HANGUL_S_FIRST                       0xAC00


#define HANGUL_S_LAST                        0xD7A3


#define HANGUL_S_COUNT                       11172 /* LCount * NCount */

#define CP_LATIN_CAPITAL_LETTER_I                 0x0049
#define CP_LATIN_CAPITAL_LETTER_J                 0x004A
#define CP_LATIN_SMALL_LETTER_I                   0x0069
#define CP_LATIN_SMALL_LETTER_J                   0x006A
#define CP_LATIN_CAPITAL_LETTER_I_WITH_GRAVE      0x00CC
#define CP_LATIN_CAPITAL_LETTER_I_WITH_ACUTE      0x00CD
#define CP_LATIN_CAPITAL_LETTER_I_WITH_TILDE      0x0128
#define CP_LATIN_CAPITAL_LETTER_I_WITH_OGONEK     0x012E
#define CP_LATIN_SMALL_LETTER_I_WITH_OGONEK       0x012F
#define CP_LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE  0x0130
#define CP_LATIN_SMALL_LETTER_DOTLESS_I           0x0131
#define CP_COMBINING_GRAVE_ACCENT                 0x0300
#define CP_COMBINING_ACUTE_ACCENT                 0x0301
#define CP_COMBINING_TILDE_ACCENT                 0x0303
#define CP_COMBINING_DOT_ABOVE                    0x0307
#define CP_COMBINING_GREEK_YPOGEGRAMMENI          0x0345
#define CP_COMBINING_GRAPHEME_JOINER              0x034F
#define CP_GREEK_CAPITAL_LETTER_SIGMA             0x03A3

#define CCC_NOT_REORDERED                         0
#define CCC_OVERLAY                               1
#define CCC_NUKTA                                 7
#define CCC_KANA_VOICING                          8
#define CCC_VIRAMA                                9
#define CCC_FIXED_POSITION_START                  10
#define CCC_FIXED_POSITION_END                    199
#define CCC_ATTACHED_BELOW_LEFT                   200
#define CCC_ATTACHED_BELOW                        202
#define CCC_ATTACHED_BOTTOM_RIGHT                 204
#define CCC_ATTACHED_LEFT                         208
#define CCC_ATTACHED_RIGHT                        210
#define CCC_ATTACHED_TOP_LEFT                     212
#define CCC_ATTACHED_ABOVE                        214
#define CCC_ATTACHED_ABOVE_RIGHT                  216
#define CCC_BELOW_LEFT                            218
#define CCC_BELOW                                 220
#define CCC_BELOW_RIGHT                           222
#define CCC_LEFT                                  224
#define CCC_RIGHT                                 226
#define CCC_ABOVE_LEFT                            228
#define CCC_ABOVE                                 230
#define CCC_ABOVE_RIGHT                           232
#define CCC_DOUBLE_BELOW                          233
#define CCC_DOUBLE_ABOVE                          234
#define CCC_IOTA_SUBSCRIPT                        240
#define CCC_INVALID                               255





#endif /* _UTF8REWIND_INTERNAL_CODEPOINT_H_ */

#ifndef _UTF8REWIND_INTERNAL_DATABASE_H_
#define _UTF8REWIND_INTERNAL_DATABASE_H_




typedef enum QuickCheckCaseMapped
{
	QuickCheckCaseMapped_Uppercase = 0x01,
	QuickCheckCaseMapped_Lowercase = 0x02,
	QuickCheckCaseMapped_Titlecase = 0x04,
	QuickCheckCaseMapped_Casefolded = 0x08,
} QuickCheckCaseMapped;

typedef enum QuickCheckResult
{
	QuickCheckResult_Yes,
	QuickCheckResult_Maybe,
	QuickCheckResult_No,
} QuickCheckResult;

#define PROPERTY_INDEX_SHIFT (5)

static const unicode_t PROPERTY_DATA_MASK = (1 << PROPERTY_INDEX_SHIFT) - 1;

#define PROPERTY_GET(_indexArray, _dataArray, _cp) \
	(_dataArray)[ \
		(_indexArray)[(_cp) >> PROPERTY_INDEX_SHIFT] + \
		((_cp) & PROPERTY_DATA_MASK)]

#define PROPERTY_GET_GC(_cp) \
	PROPERTY_GET(GeneralCategoryIndexPtr, GeneralCategoryDataPtr, _cp)

#define PROPERTY_GET_CCC(_cp) \
	PROPERTY_GET(CanonicalCombiningClassIndexPtr, CanonicalCombiningClassDataPtr, _cp)

#define PROPERTY_GET_CM(_cp) \
	PROPERTY_GET(QuickCheckCaseMappedIndexPtr, QuickCheckCaseMappedDataPtr, _cp)

#define PROPERTY_GET_NFC(_cp) \
	PROPERTY_GET(QuickCheckNFCIndexPtr, QuickCheckNFCDataPtr, _cp)

#define PROPERTY_GET_NFD(_cp) \
	PROPERTY_GET(QuickCheckNFDIndexPtr, QuickCheckNFDDataPtr, _cp)

#define PROPERTY_GET_NFKC(_cp) \
	PROPERTY_GET(QuickCheckNFKCIndexPtr, QuickCheckNFKCDataPtr, _cp)

#define PROPERTY_GET_NFKD(_cp) \
	PROPERTY_GET(QuickCheckNFKDIndexPtr, QuickCheckNFKDDataPtr, _cp)



#endif /* _UTF8REWIND_INTERNAL_DATABASE_H_ */


#ifndef _UTF8REWIND_INTERNAL_STREAMING_H_
#define _UTF8REWIND_INTERNAL_STREAMING_H_




/*
	UAX15-D4. Stream-Safe Text Process
		
	This is the process of producing a Unicode string in Stream-Safe Text Format by processing that string
	from start to finish, inserting U+034F COMBINING GRAPHEME JOINER (CGJ) within long sequences of
	non-starters. The exact position of the inserted CGJs are determined according to the following algorithm,
	which describes the generation of an output string from an input string:

	* If the input string is empty, return an empty output string.
	* Set nonStarterCount to zero.
	* For each code point C in the input string:
		* Produce the NFKD decomposition S.
		* If nonStarterCount plus the number of initial non-starters in S is greater than 30, append a CGJ to
			the output string and set the nonStarterCount to zero.
		* Append C to the output string.
		* If there are no starters in S, increment nonStarterCount by the number of code points in S; otherwise,
			set nonStarterCount to the number of trailing non-starters in S (which may be zero).
	* Return the output string.
*/

#define STREAM_SAFE_MAX 30
#define STREAM_BUFFER_MAX 32

typedef struct {
	const char* src;
	size_t src_size;
	uint8_t index;
	uint8_t current;
	uint8_t filled;
	uint8_t stable;
	uint8_t last_length;
	unicode_t codepoint[STREAM_BUFFER_MAX];
	uint8_t quick_check[STREAM_BUFFER_MAX];
	uint8_t canonical_combining_class[STREAM_BUFFER_MAX];
} StreamState;




#endif /* _UTF8REWIND_INTERNAL_STREAMING_H_ */

#ifndef _UTF8REWIND_INTERNAL_COMPOSITION_H_
#define _UTF8REWIND_INTERNAL_COMPOSITION_H_




typedef struct {
	StreamState* input;
	StreamState* output;
	const size_t* qc_index;
	const uint8_t* qc_data;
} ComposeState;



#endif /* _UTF8REWIND_INTERNAL_COMPOSITION_H_ */

#ifndef _UTF8REWIND_INTERNAL_DECOMPOSITION_H_
#define _UTF8REWIND_INTERNAL_DECOMPOSITION_H_




typedef struct {
	StreamState* input;
	StreamState* output;
	const size_t* qc_index;
	const uint8_t* qc_data;
	const uint32_t* property_index1;
	const uint32_t* property_index2;
	const uint32_t* property_data;
	unicode_t cache_codepoint[STREAM_BUFFER_MAX];
	uint8_t cache_canonical_combining_class[STREAM_BUFFER_MAX];
	uint8_t cache_current;
	uint8_t cache_filled;
} DecomposeState;



#endif /* _UTF8REWIND_INTERNAL_DECOMPOSITION_H_ */

#ifndef _UTF8REWIND_UNICODEDATABASE_H_
#define _UTF8REWIND_UNICODEDATABASE_H_




typedef struct {
	unicode_t codepoint;
	uint32_t length_and_offset;
} DecompositionRecord;

typedef struct {
	uint64_t key;
	unicode_t value;
} CompositionRecord;



#endif /* _UTF8REWIND_UNICODEDATABASE_H_ */


#endif /*U8_H*/
