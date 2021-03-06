#include "operations.h"
#include "language.h"
#include "lib/debug.h"

// Relates to ToNumber() in spec
// https://www.ecma-international.org/ecma-262/5.1/#sec-9.3
ToNumberResult jsValueToNumber(JsValue* value) {
    switch(jsValueType(value)) {
        case NUMBER_TYPE:
            return (ToNumberResult){
                .number = jsValueNumber(value),
            };
        case NULL_TYPE:
            return (ToNumberResult){
                .isZero = true,
            };
        case NAN_TYPE:
        case UNDEFINED_TYPE:
            return (ToNumberResult){
                .isNaN = true,
            };
        case BOOLEAN_TYPE:
            if(value == getTrue()) {
                return (ToNumberResult){
                    .isOne = true,
                };
            } else {
                return (ToNumberResult){
                    .isZero = true,
                };
            }
        default:
            fail("Must implement jsValueNumber for %s", jsValueReflect(value).name);
            // placeholder, fail exits
            return (ToNumberResult){
                .isNaN = true
            };
    }
}
