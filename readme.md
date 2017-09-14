# lock-free hashmap
Concerning the lock-free hashmap of C, due to MSVC's lack of support for C11, it's written in C++11.

# build
## MSVC 
    cmake -G"Visual Studio 15 2017 Win64" ../lockfree_hashmap
## Linux/Mac make
    cmake ../lockfree_hashmap
## Mac Xcode
    cmake -G"Xcode" ../lockfree_hashmap
    
    