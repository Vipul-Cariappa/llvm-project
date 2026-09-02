// RUN: %clang_cc1 -fincremental-extensions -E %s
// RUN: %clang_cc1 -fincremental-extensions -fparse-functions-ondemand -E %s
// RUN: %clang_cc1 -fincremental-extensions -E -dD %s
// RUN: %clang_cc1 -fincremental-extensions -fparse-functions-ondemand -E -dD %s
// RUN: %clang_cc1 -fincremental-extensions -E -dI %s
// RUN: %clang_cc1 -fincremental-extensions -fparse-functions-ondemand -E -dI %s
// RUN: %clang_cc1 -fincremental-extensions -E -dM %s
// RUN: %clang_cc1 -fincremental-extensions -fparse-functions-ondemand -E -dM %s
