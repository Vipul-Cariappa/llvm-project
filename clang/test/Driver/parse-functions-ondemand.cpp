// RUN: %clangxx -### --target=x86_64-unknown-linux-gnu -fparse-functions-ondemand -c %s 2>&1 | FileCheck %s

// CHECK: "-fparse-functions-ondemand"
