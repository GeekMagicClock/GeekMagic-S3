
#define PRODUCT_MODEL "GeekMagic S3"
#define SW_VERSION  "V1.0.0"

//#define DISABLE_ALL_PRINT  // 若要禁用所有打印，取消注释此行
#define ENABLE_ERROR_PRINT  // 若要启用错误打印，取消注释此行
#define ENABLE_DEBUG_PRINT  // 若要启用调试打印，取消注释此行

#ifdef DISABLE_ALL_PRINT
#define ERR_PTN(x)
#define DBG_PTN(x)
#else
#ifdef ENABLE_ERROR_PRINT
#define ERR_PTN(x) Serial.println(x)
#else
#define ERR_PTN(x)
#endif

#ifdef ENABLE_DEBUG_PRINT
#define DBG_PTN(x) Serial.println(x)
#else
#define DBG_PTN(x)
#endif
#endif