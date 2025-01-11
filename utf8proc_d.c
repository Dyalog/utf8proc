/******************************************************************************
* This Dyalog-specific file is used to define versions of
* utf8proc_decompose_custom() which take 8-, 16- and 32-bit character /str/
* arguments, rather than UTF-8 strings.
* The function was moved from utf8proc.c to here, and modified slightly.
******************************************************************************/
#ifdef DYALOG_CHARTYPE
# define DYALOG_UTF8		0
# define DYALOG_FN2(TYPE)	utf8proc_decompose_custom_ ## TYPE
# define DYALOG_FN(TYPE)	DYALOG_FN2(TYPE)
#else // def DYALOG_CHARTYPE
# define DYALOG_UTF8		1
# define DYALOG_CHARTYPE	utf8proc_uint8_t
# define DYALOG_FN(TYPE)	utf8proc_decompose_custom
#endif // def DYALOG_CHARTYPE

UTF8PROC_DLLEXPORT utf8proc_ssize_t DYALOG_FN (DYALOG_CHARTYPE) (
  const DYALOG_CHARTYPE *str, utf8proc_ssize_t strlen,
  utf8proc_int32_t *buffer, utf8proc_ssize_t bufsize, utf8proc_option_t options,
  utf8proc_custom_func custom_func, void *custom_data
) {
  /* strlen will be ignored, if UTF8PROC_NULLTERM is set in options */
  utf8proc_ssize_t wpos = 0;
  if ((options & UTF8PROC_COMPOSE) && (options & UTF8PROC_DECOMPOSE))
    return UTF8PROC_ERROR_INVALIDOPTS;
  if ((options & UTF8PROC_STRIPMARK) &&
      !(options & UTF8PROC_COMPOSE) && !(options & UTF8PROC_DECOMPOSE))
    return UTF8PROC_ERROR_INVALIDOPTS;
  {
    utf8proc_int32_t uc;
    utf8proc_ssize_t rpos = 0;
    utf8proc_ssize_t decomp_result;
    int boundclass = UTF8PROC_BOUNDCLASS_START;
    while (1) {
#if DYALOG_UTF8
      if (options & UTF8PROC_NULLTERM) {
        rpos += utf8proc_iterate(str + rpos, -1, &uc);
        /* checking of return value is not necessary,
           as 'uc' is < 0 in case of error */
        if (uc < 0) return UTF8PROC_ERROR_INVALIDUTF8;
        if (rpos < 0) return UTF8PROC_ERROR_OVERFLOW;
        if (uc == 0) break;
      } else {
        if (rpos >= strlen) break;
        rpos += utf8proc_iterate(str + rpos, strlen - rpos, &uc);
        if (uc < 0) return UTF8PROC_ERROR_INVALIDUTF8;
      }
#else // DYALOG_UTF8
      if (rpos >= strlen) break;
      uc = str[rpos++];
#endif // DYALOG_UTF8
      if (custom_func != NULL) {
        uc = custom_func(uc, custom_data);   /* user-specified custom mapping */
      }
      decomp_result = utf8proc_decompose_char(
        uc, buffer + wpos, (bufsize > wpos) ? (bufsize - wpos) : 0, options,
        &boundclass
      );
      if (decomp_result < 0) return decomp_result;
      wpos += decomp_result;
      /* prohibiting integer overflows due to too long strings: */
      if (wpos < 0 ||
          wpos > (utf8proc_ssize_t)(SSIZE_MAX/sizeof(utf8proc_int32_t)/2))
        return UTF8PROC_ERROR_OVERFLOW;
    }
  }
  if ((options & (UTF8PROC_COMPOSE|UTF8PROC_DECOMPOSE)) && bufsize >= wpos) {
    utf8proc_ssize_t pos = 0;
    while (pos < wpos-1) {
      utf8proc_int32_t uc1, uc2;
      const utf8proc_property_t *property1, *property2;
      uc1 = buffer[pos];
      uc2 = buffer[pos+1];
      property1 = unsafe_get_property(uc1);
      property2 = unsafe_get_property(uc2);
      if (property1->combining_class > property2->combining_class &&
          property2->combining_class > 0) {
        buffer[pos] = uc2;
        buffer[pos+1] = uc1;
        if (pos > 0) pos--; else pos++;
      } else {
        pos++;
      }
    }
  }
  return wpos;
}

#undef DYALOG_FN
#undef DYALOG_FN2
#undef DYALOG_UTF8
#undef DYALOG_CHARTYPE
