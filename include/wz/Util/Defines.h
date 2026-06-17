#ifndef WZ_UTIL_DEFINES_H_
#define WZ_UTIL_DEFINES_H_

#define WZ_DISALLOW_COPY(TypeName)                                             \
  TypeName(const TypeName&) = delete;                                          \
  TypeName& operator=(const TypeName&) = delete;

#define WZ_DISALLOW_MOVE(TypeName)                                             \
  TypeName(TypeName&&) = delete;                                               \
  TypeName& operator=(TypeName&&) = delete;

#define WZ_DISALLOW_COPY_AND_MOVE(TypeName)                                    \
  WZ_DISALLOW_COPY(TypeName)                                                   \
  WZ_DISALLOW_MOVE(TypeName)

#endif  // WZ_UTIL_DEFINES_H_
