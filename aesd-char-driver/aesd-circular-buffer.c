/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @brief Provides the number of current elements present in the circular buffer
 * 
 * @param buffer 
 * @return number of elements 
 */
int8_t aesd_buffer_size(struct aesd_circular_buffer *buffer) {
  int8_t size = buffer->in_offs - buffer->out_offs;

  if (0 > size || (0 == size && buffer->full))
    size += AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

  return size;
}

/**
 * @brief Return the total sum of all data elements in the buffer
 * 
 * @param buffer 
 * @return total size_t from all elements 
 */
size_t aesd_circular_buffer_data_length(aesd_cbuff_t *buffer) {
  size_t length = 0;

  if (NULL != buffer){

    int8_t n_elem = aesd_buffer_size(buffer);

    int i;
    for (i = 0; i < n_elem; ++i) {
      uint8_t offset =
          (buffer->out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

      length += buffer->entry[offset].size;

    }
  }
  return length;
}

/**
 * @brief Returns the base offset from the head (out_offs). Assumtion: caller
 * already valided with `aesd_circular_buffer_is_valid_seekto`
 *
 * @param buffer
 * @param wr_cmd
 * @param wr_cmd_offset
 * @return size_t
 */
size_t aesd_circular_buffer_base_offset(aesd_cbuff_t *buffer, uint32_t const wr_cmd,
                                        uint32_t const wr_cmd_offset) {
  size_t base_offset = 0;

  if (NULL != buffer) {

    int8_t const n_elem = aesd_buffer_size(buffer);
    if (n_elem < wr_cmd)
      return 0L;

    int i;
    for (i = 0; i < n_elem; ++i) {
      uint8_t cmd_offset =
          (buffer->out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

      if (wr_cmd == i) {
        base_offset += wr_cmd_offset;
        break;
      }
      base_offset += buffer->entry[cmd_offset].size;
    }
  }
  return base_offset;
}

/**
 * @brief Check if the cmd and offset are within a valid "range"
 *
 * @param buffer
 * @param wr_cmd cmd offset from the current head (out_offs)
 * @param wr_cmd_offset from the wr_cmd element
 * @return true if valid, false otherwise.
 */
bool aesd_circular_buffer_is_valid_seekto(aesd_cbuff_t *buffer, uint32_t wr_cmd,
                                          uint32_t wr_cmd_offset) {
  bool is_valid = false;

  if (NULL != buffer) {

    int8_t const n_elem = aesd_buffer_size(buffer);

    if (n_elem >= wr_cmd) {
      uint8_t offset =
          (buffer->out_offs + wr_cmd) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
      is_valid = (wr_cmd_offset <= buffer->entry[offset].size) ? true : false;
    }
  }
  return is_valid;
}
/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    if ((NULL == buffer) || (NULL == entry_offset_byte_rtn))
      return NULL;
    struct aesd_buffer_entry *ret_val = NULL;

    int8_t n_elem = aesd_buffer_size(buffer);

    size_t count = 0;
    int i;
    for (i = 0; i < n_elem; ++i) {
      uint8_t offset = (buffer->out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

      count += buffer->entry[offset].size;

      if (count > char_offset) {
        *entry_offset_byte_rtn =
            (buffer->entry[offset].size - (count - char_offset));

        ret_val = &buffer->entry[offset];
        break;
      }
    }
    return ret_val;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    if ((NULL == buffer) || (NULL == add_entry))
      return;

    if (buffer->full)
      buffer->out_offs =
          (++buffer->out_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
              ? 0
              : buffer->out_offs;

    buffer->entry[buffer->in_offs] = *add_entry;

    buffer->in_offs =
        (++buffer->in_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
            ? 0
            : buffer->in_offs;
    // Check if full
    if (buffer->in_offs == buffer->out_offs) buffer->full = true;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
