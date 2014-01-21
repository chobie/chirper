#ifndef PHP_CHIRPER_H
#define PHP_CHIRPER_H

#define PHP_CHIRPER_EXTNAME "chirper"
#define PHP_CHIRPER_EXTVER "0.2.0"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "ext/standard/php_smart_str.h"
#include "ext/spl/spl_exceptions.h"
#include "ext/standard/php_var.h"
#include "ext/standard/php_string.h"
#include "ext/standard/php_incomplete_class.h"
#include "ext/standard/info.h"
#include "ext/standard/php_array.h"
#include "limits.h"

#include "date/php_date.h"

#include <stdlib.h>

/* Define the entry point symbol
 * Zend will use when loading this module
 */
extern zend_module_entry chirper_module_entry;
#define phpext_chirper_ptr &chirper_module_entry

ZEND_BEGIN_MODULE_GLOBALS(chirper)
	long dummy;
ZEND_END_MODULE_GLOBALS(chirper)

ZEND_EXTERN_MODULE_GLOBALS(chirper)

#ifdef ZTS
#define CHIRPERG(v) TSRMG(chirper_globals_id, zend_chirper_globals *, v)
#else
#define CHIRPERG(v) (chirper_globals.v)
#endif

#endif /* PHP_CHIRPER_H */