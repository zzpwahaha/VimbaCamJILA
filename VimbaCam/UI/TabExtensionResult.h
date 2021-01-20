#ifndef TAB_EXTENSION_RESULT_H_
#define TAB_EXTENSION_RESULT_H_
/*Results that will be returned from plugin functions*/
enum TabExtensionResult
{
    TER_OK              = 0,
    TER_RuntimeError    ,
    TER_OutOfMemory     ,
    TER_OutOfRange      ,
    TER_NotSupported    ,
    TER_BadParameter    ,
};
#endif