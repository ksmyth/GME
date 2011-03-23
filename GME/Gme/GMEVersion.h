// Automatically generated from GMEVersion.h.tmpl by Install\Build\build.py

#if !defined(_GMEVERSION_H_)
#define _GMEVERSION_H_

#define GME_VERSION_MAJOR	11		// MAJOR = Last two digits of Year
#define GME_VERSION_MINOR	3		// MINOR = Month
#define GME_VERSION_PLEVEL	23		// PATCH LEVEL = Day

#define _VERSION_STRING2(x)	#x 
#define _VERSION_STRING(x)	_VERSION_STRING2(x)
#define _VERSION_STRING_CON2(x, y)	_VERSION_STRING2(x##y)
#define _VERSION_STRING_CON(x, y)	_VERSION_STRING_CON2(x, y)

#define GME_VERSION_STR _VERSION_STRING(GME_VERSION_MAJOR.GME_VERSION_MINOR.GME_VERSION_PLEVEL)

#define GME_VERSION_ID	_VERSION_STRING_CON(Release r, GME_VERSION_MAJOR.GME_VERSION_MINOR.GME_VERSION_PLEVEL)


#endif // !defined(_GMEVERSION_H_)
