#pragma once
// CPLEXConfig.h
#pragma once

#ifndef CPLEX_CONFIG_H
#define CPLEX_CONFIG_H

// ========== 防止重复包含导致的宏重定义 ==========
#ifdef NOMINMAX
#undef NOMINMAX
#endif
#define NOMINMAX

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#define WIN32_LEAN_AND_MEAN

// ========== 清除可能冲突的宏 ==========
#ifdef ImplClass
#undef ImplClass
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef ERROR
#undef ERROR
#endif

// ========== 确保 IL_STD 被定义 ==========
#ifndef IL_STD
#define IL_STD
#endif

// ========== 包含 CPLEX 头文件 ==========
#include <ilcplex/ilocplex.h>

#endif // CPLEX_CONFIG_H