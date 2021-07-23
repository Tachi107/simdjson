#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

class array;
class object;
class value;
class raw_json_string;
class document_stream;

/**
 * The default batch size for document_stream instances for this On Demand kernel.
 * Note that different On Demand kernel may use a different DEFAULT_BATCH_SIZE value
 * in the future.
 */
static constexpr size_t DEFAULT_BATCH_SIZE = 1000000;
/**
 * Some adversary might try to set the batch size to 0 or 1, which might cause problems.
 * We set a minimum of 32B since anything else is highly likely to be an error. In practice,
 * most users will want a much larger batch size.
 *
 * All non-negative MINIMAL_BATCH_SIZE values should be 'safe' except that, obviously, no JSON
 * document can ever span 0 or 1 byte and that very large values would create memory allocation issues.
 */
static constexpr size_t MINIMAL_BATCH_SIZE = 32;

/**
 * A JSON fragment iterator.
 *
 * This holds the actual iterator as well as the buffer for writing strings.
 */
class parser {
public:
  /**
   * Create a JSON parser.
   *
   * The new parser will have zero capacity.
   */
  inline explicit parser(size_t max_capacity = SIMDJSON_MAXSIZE_BYTES) noexcept;

  inline parser(parser &&other) noexcept = default;
  simdjson_really_inline parser(const parser &other) = delete;
  simdjson_really_inline parser &operator=(const parser &other) = delete;
  simdjson_really_inline parser &operator=(parser &&other) noexcept = default;

  /** Deallocate the JSON parser. */
  inline ~parser() noexcept = default;

  /**
   * Start iterating an on-demand JSON document.
   *
   *   ondemand::parser parser;
   *   document doc = parser.iterate(json);
   *
   * The input buffer does not need to be padded.
   *
   * ### IMPORTANT: Validate what you use
   *
   * Calling iterate on an invalid JSON document may not immediately trigger an error. The call to
   * iterate does not parse and validate the whole document.
   *
   * ### IMPORTANT: Buffer Lifetime
   *
   * Because parsing is done while you iterate, you *must* keep the JSON buffer around at least as
   * long as the document iteration.
   *
   * ### IMPORTANT: Document Lifetime
   *
   * Only one iteration at a time can happen per parser, and the parser *must* be kept alive during
   * iteration to ensure intermediate buffers can be accessed. Any document must be destroyed before
   * you call parse() again or destroy the parser.
   *
   *
   * @param json The JSON to parse.
   * @param len The length of the JSON.
   *
   * @return The document, or an error:
   *         - MEMALLOC if realloc_if_needed the parser does not have enough capacity, and memory
   *           allocation fails.
   *         - EMPTY if the document is all whitespace.
   *         - UTF8_ERROR if the document is not valid UTF-8.
   *         - UNESCAPED_CHARS if a string contains control characters that must be escaped
   *         - UNCLOSED_STRING if there is an unclosed string in the document.
   */
  simdjson_warn_unused simdjson_result<document> iterate(const char *json, size_t len) & noexcept;
  /** @overload simdjson_result<document> iterate(const char *json, size_t len) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(std::string_view view) & noexcept;
  /** @overload simdjson_result<document> iterate(const char *json, size_t len) & noexcept */
  simdjson_warn_unused simdjson_result<document> iterate(const std::string &json) & noexcept;

  /**
   * @overload simdjson_result<document> iterate(const char *json, size_t len) & noexcept;
   * iterate does not require padding, this function is kept for backward compatibility and convenience.
   **/
  simdjson_warn_unused simdjson_result<document> iterate(padded_string_view json) & noexcept;

  /**
   * @overload simdjson_result<document> iterate(const char *json, size_t len) & noexcept;
   * iterate does not require padding, this function is kept for backward compatibility and convenience.
   **/
  simdjson_warn_unused simdjson_result<document> iterate(const padded_string & json) & noexcept;

  /**
   * @overload simdjson_result<document> iterate(const char *json, size_t len) & noexcept;
   * iterate does not require padding, this function is kept for backward compatibility and convenience.
   **/
  simdjson_warn_unused simdjson_result<document> iterate(const char *json, size_t len, size_t capacity) & noexcept;

  /**
   * @overload simdjson_result<document> iterate(const char *json, size_t len) & noexcept;
   * iterate does not require padding, this function is kept for backward compatibility and convenience.
   **/
  simdjson_warn_unused simdjson_result<document> iterate(const uint8_t *json, size_t len, size_t capacity) & noexcept;

  /**
   * @overload simdjson_result<document> iterate(const char *json, size_t len) & noexcept;
   * iterate does not require padding, this function is kept for backward compatibility and convenience.
   **/
  simdjson_warn_unused simdjson_result<document> iterate(std::string_view json, size_t capacity) & noexcept;

  /**
   * @overload simdjson_result<document> iterate(const char *json, size_t len) & noexcept;
   * iterate does not require padding, this function is kept for backward compatibility and convenience.
   **/
  simdjson_warn_unused simdjson_result<document> iterate(const simdjson_result<padded_string> &json) & noexcept;

  /**
   * @overload simdjson_result<document> iterate(const char *json, size_t len) & noexcept;
   * iterate does not require padding, this function is kept for backward compatibility and convenience.
   **/
  simdjson_warn_unused simdjson_result<document> iterate(const simdjson_result<padded_string_view> &json) & noexcept;

  /**
   * @overload simdjson_result<document> iterate(const char *json, size_t len) & noexcept;
   * iterate does not require padding, this function is kept for backward compatibility and convenience.
   **/
  simdjson_warn_unused simdjson_result<document> iterate(padded_string &&json) & noexcept = delete;
  /**
   * @overload simdjson_result<document> iterate(const char *json, size_t len) & noexcept;
   * iterate does not require padding, this function is kept for backward compatibility and convenience.
   **/
  simdjson_warn_unused simdjson_result<document> iterate(std::string &&json) & noexcept = delete;


  /**
   * @private
   */
  simdjson_warn_unused simdjson_result<json_iterator> iterate_raw(padded_string_view json) & noexcept;


  /**
   * Parse a buffer containing many JSON documents.
   *
   *   std::string json = R"({ "foo": 1 } { "foo": 2 } { "foo": 3 } )";
   *   ondemand::parser parser;
   *   ondemand::document_stream docs = parser.iterate_many(json);
   *   for (auto & doc : docs) {
   *     std::cout << doc["foo"] << std::endl;
   *   }
   *   // Prints 1 2 3
   *
   * No copy of the input buffer is made.
   *
   * The function is lazy: it may be that no more than one JSON document at a time is parsed.

   *
   * The caller is responsabile to ensure that the input string data remains unchanged and is
   * not deleted during the loop.
   * ### Format
   *
   * The buffer must contain a series of one or more JSON documents, concatenated into a single
   * buffer, separated by whitespace. It effectively parses until it has a fully valid document,
   * then starts parsing the next document at that point. (It does this with more parallelism and
   * lookahead than you might think, though.)
   *
   * documents that consist of an object or array may omit the whitespace between them, concatenating
   * with no separator. documents that consist of a single primitive (i.e. documents that are not
   * arrays or objects) MUST be separated with whitespace.
   *
   * The documents must not exceed batch_size bytes (by default 1MB) or they will fail to parse.
   * Setting batch_size to excessively large or excesively small values may impact negatively the
   * performance.
   *
   * ### Threads
   *
   * When compiled with SIMDJSON_THREADS_ENABLED, this method will use a single thread under the
   * hood to do some lookahead.
   *
   * ### Parser Capacity
   *
   * If the parser's current capacity is less than batch_size, it will allocate enough capacity
   * to handle it (up to max_capacity).
   *
   * @param buf The concatenated JSON to parse.
   * @param len The length of the concatenated JSON.
   * @param batch_size The batch size to use. MUST be larger than the largest document. The sweet
   *                   spot is cache-related: small enough to fit in cache, yet big enough to
   *                   parse as many documents as possible in one tight loop.
   *                   Defaults to 10MB, which has been a reasonable sweet spot in our tests.
   * @return The stream, or an error. An empty input will yield 0 documents rather than an EMPTY error. Errors:
   *         - MEMALLOC if the parser does not have enough capacity and memory allocation fails
   *         - CAPACITY if the parser does not have enough capacity and batch_size > max_capacity.
   *         - other json errors if parsing fails. You should not rely on these errors to always the same for the
   *           same document: they may vary under runtime dispatch (so they may vary depending on your system and hardware).
   */
  inline simdjson_result<document_stream> iterate_many(std::string_view view, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  inline simdjson_result<document_stream> iterate_many(const uint8_t *buf, size_t len, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline simdjson_result<document_stream> iterate_many(const char *buf, size_t len, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline simdjson_result<document_stream> iterate_many(const std::string &s, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  inline simdjson_result<document_stream> iterate_many(const std::string &&s, size_t batch_size) = delete;// unsafe
  /** @overload parse_many(const uint8_t *buf, size_t len, size_t batch_size) */
  inline simdjson_result<document_stream> iterate_many(const padded_string &s, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept;
  inline simdjson_result<document_stream> iterate_many(const padded_string &&s, size_t batch_size) = delete;// unsafe

  /** @private We do not want to allow implicit conversion from C string to std::string. */
  simdjson_result<document_stream> iterate_many(const char *buf, size_t batch_size = DEFAULT_BATCH_SIZE) noexcept = delete;

  /** The capacity of this parser (the largest document it can process). */
  simdjson_really_inline size_t capacity() const noexcept;
  /** The maximum capacity of this parser (the largest document it is allowed to process). */
  simdjson_really_inline size_t max_capacity() const noexcept;
  simdjson_really_inline void set_max_capacity(size_t max_capacity) noexcept;
  /** The maximum depth of this parser (the most deeply nested objects and arrays it can process). */
  simdjson_really_inline size_t max_depth() const noexcept;

  /**
   * Ensure this parser has enough memory to process JSON documents up to `capacity` bytes in length
   * and `max_depth` depth.
   *
   * @param capacity The new capacity.
   * @param max_depth The new max_depth. Defaults to DEFAULT_MAX_DEPTH.
   * @return The error, if there is one.
   */
  simdjson_warn_unused error_code allocate(size_t capacity, size_t max_depth=DEFAULT_MAX_DEPTH) noexcept;

  #ifdef SIMDJSON_THREADS_ENABLED
  /**
   * The parser instance can use threads when they are available to speed up some
   * operations. It is enabled by default. Changing this attribute will change the
   * behavior of the parser for future operations.
   */
  bool threaded{true};
  #endif

private:
  /** @private [for benchmarking access] The implementation to use */
  std::unique_ptr<internal::dom_parser_implementation> implementation{};
  size_t _capacity{0};
  size_t _max_capacity;
  size_t _max_depth{DEFAULT_MAX_DEPTH};
  std::unique_ptr<uint8_t[]> string_buf{};
#ifdef SIMDJSON_DEVELOPMENT_CHECKS
  std::unique_ptr<token_position[]> start_positions{};
#endif

  friend class json_iterator;
  friend class document_stream;
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::parser> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::parser> {
public:
  simdjson_really_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::parser &&value) noexcept; ///< @private
  simdjson_really_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_really_inline simdjson_result() noexcept = default;
};

} // namespace simdjson
