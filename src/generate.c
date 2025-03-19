#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define global_variable static
#define strlit_len(s) (sizeof(s) - 1)
#define strlit(s) (String){ .pointer = (u8*)(s), .length = strlit_len(s), }

#if _MSC_VER
#define trap() __fastfail(1)
#elif __has_builtin(__builtin_trap)
#define trap() __builtin_trap()
#else
extern BB_NORETURN BB_COLD void abort(void);
fn BB_NORETURN BB_COLD void trap_ext()
{
#ifdef __x86_64__
    asm volatile("ud2");
#else
    abort();
#endif
}
#define trap() (trap_ext(), __builtin_unreachable())
#endif

#ifndef BB_DEBUG
#define BB_DEBUG 1
#endif

#ifndef BB_SAFETY
#define BB_SAFETY BB_DEBUG
#endif

#define unused(x) (void)(x)

#if _MSC_VER
#define BB_NORETURN __declspec(noreturn)
#define BB_COLD __declspec(noinline)
#elif defined(__TINYC__)
#define BB_NORETURN __attribute__((noreturn))
#define BB_COLD __attribute__((cold))
#else
#define BB_NORETURN [[noreturn]]
#define BB_COLD [[gnu::cold]]
#endif


#if _MSC_VER
#define expect(x, b) (!!(x))
#else
#define expect(x, b) __builtin_expect(!!(x), b)
#endif
#define likely(x) expect(x, 1)
#define unlikely(x) expect(x, 0)
#define panic(format, ...) (os_exit(1))

#if BB_DEBUG
#define assert(x) (unlikely(!(x)) ? panic("Assert failed: \"" # x "\" at {cstr}:{u32}\n", __FILE__, __LINE__) : unused(0))
#else
#define assert(x) unused(likely(x))
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#define fn static

#define KB(n) ((n) * 1024)
#define MB(n) ((n) * 1024 * 1024)
#define GB(n) ((u64)(n) * 1024 * 1024 * 1024)
#define TB(n) ((u64)(n) * 1024 * 1024 * 1024 * 1024)


#define STRUCT_FORWARD_DECL(S) typedef struct S S
#define STRUCT(S) STRUCT_FORWARD_DECL(S); struct S
#define UNION_FORWARD_DECL(U) typedef union U U
#define UNION(U) UNION_FORWARD_DECL(U); union U

#define Slice(T) Slice_ ## T
#define SliceP(T) SliceP_ ## T
#define declare_slice_ex(T, StructName) STRUCT(StructName) \
{\
    T* pointer;\
    u64 length;\
}

#define declare_slice(T) declare_slice_ex(T, Slice(T))
#define declare_slice_p(T) declare_slice_ex(T*, SliceP(T))

declare_slice(u8);
typedef Slice(u8) String;

#define VirtualBuffer(T) VirtualBuffer_ ## T
#define VirtualBufferP(T) VirtualBufferPointerTo_ ## T

#define decl_vb_ex(T, StructName) \
struct StructName \
{\
    T* pointer;\
    u32 length;\
    u32 capacity;\
};\
typedef struct StructName StructName

#define decl_vb(T) decl_vb_ex(T, VirtualBuffer(T))
#define decl_vbp(T) decl_vb_ex(T*, VirtualBufferP(T))

decl_vb(u8);
decl_vbp(u8);
decl_vb(u16);
decl_vbp(u16);
decl_vb(u32);
decl_vbp(u32);

#define vb_size_of_element(vb) sizeof(*((vb)->pointer))
#define vb_add(vb, count) (typeof((vb)->pointer)) vb_generic_add((VirtualBuffer(u8)*)(vb), (vb_size_of_element(vb)), (count))
#define vb_add_scalar(vb, S) (S*) vb_generic_add(vb, 1, sizeof(S))
#define vb_copy_scalar(vb, s) *vb_add_scalar(vb, typeof(s)) = s
#define vb_append_struct(vb, T, s) *(vb_add_struct(vb, T)) = s
#define vb_append_one(vb, item) (typeof((vb)->pointer)) vb_generic_append((VirtualBuffer(u8)*)(vb), &(item), (vb_size_of_element(vb)), 1)
#define vb_to_bytes(vb) (Slice(u8)) { .pointer = (u8*)((vb).pointer), .length = (vb_size_of_element(vb)) * (vb).length, }
#define vb_ensure_capacity(vb, count) vb_generic_ensure_capacity((VirtualBuffer(u8)*)(vb), vb_size_of_element(vb), (count))
#define vb_copy_array(vb, arr) memcpy(vb_add(vb, array_length(arr)), arr, sizeof(arr))
#define vb_add_any_array(vb, E, count) (E*)vb_generic_add(vb, vb_size_of_element(vb), sizeof(E) * count)
#define vb_copy_any_array(vb, arr) memcpy(vb_generic_add(vb, vb_size_of_element(vb), sizeof(arr)), (arr), sizeof(arr))
#define vb_copy_any_slice(vb, slice) memcpy(vb_generic_add(vb, vb_size_of_element(vb), sizeof(*((slice).pointer)) * (slice).length), (slice).pointer, sizeof(*((slice).pointer)) * (slice).length)

fn void vb_generic_ensure_capacity(VirtualBuffer(u8)* vb, u32 item_size, u32 item_count);
fn u8* vb_generic_add_assume_capacity(VirtualBuffer(u8)* vb, u32 item_size, u32 item_count);
fn u8* vb_generic_add(VirtualBuffer(u8)* vb, u32 item_size, u32 item_count);
fn u8* vb_append_bytes(VirtualBuffer(u8*) vb, Slice(u8) bytes);
fn u32 vb_copy_string(VirtualBuffer(u8)* buffer, String string);
fn u64 vb_copy_string_zero_terminated(VirtualBuffer(u8)* buffer, String string);

STRUCT(OSReserveProtectionFlags)
{
    u32 read:1;
    u32 write:1;
    u32 execute:1;
    u32 reserved:29;
};

STRUCT(OSReserveMapFlags)
{
    u32 priv:1;
    u32 anon:1;
    u32 noreserve:1;
    u32 reserved:29;
};

#define let_pointer_cast(PointerChildType, var_name, value) PointerChildType* var_name = (PointerChildType*)(value)
#if defined(__TINYC__) || defined(_MSC_VER)
#define let(name, value) typeof(value) name = (value)
#else
#define let(name, value) __auto_type name = (value)
#endif
#define let_cast(T, name, value) T name = cast_to(T, value)
#define assign_cast(to, from) to = cast_to(typeof(to), from)
#define let_va_arg(T, name, args) T name = va_arg(args, T)
#define transmute(D, source) *(D*)&source

#if BB_SAFETY
#define cast_to(T, value) (assert((typeof(value)) (T) (value) == (value) && ((value) > 0) == ((T) (value) > 0)), (T) (value))
#else
#define cast_to(T, value) (T)(value)
#endif

#ifndef __APPLE__
#define MY_PAGE_SIZE KB(4)
#else
#define MY_PAGE_SIZE KB(16)
#endif
global_variable const u64 page_size = MY_PAGE_SIZE;

global_variable u64 minimum_granularity = MY_PAGE_SIZE;
// global_variable u64 middle_granularity = MB(2);
global_variable u64 default_size = GB(4);

fn u64 align_forward(u64 value, u64 alignment)
{
    u64 mask = alignment - 1;
    u64 result = (value + mask) & ~mask;
    return result;
}

fn u8 os_is_being_debugged()
{
    u8 result = 0;
#if _WIN32
    result = IsDebuggerPresent() != 0;
#else
#ifdef __APPLE__
    let(request, PT_TRACE_ME);
#else
    let(request, PTRACE_TRACEME);
#endif
    if (ptrace(request, 0, 0, 0) == -1)
    {
        let(error, errno);
        if (error == EPERM)
        {
            result = 1;
        }
    }
#endif

    return result;
}

BB_NORETURN BB_COLD fn void os_exit(u32 exit_code)
{
    if (exit_code != 0 && os_is_being_debugged())
    {
        trap();
    }
    _exit(exit_code);
}

fn u8* os_reserve(u64 base, u64 size, OSReserveProtectionFlags protection, OSReserveMapFlags map)
{
#if _WIN32
    DWORD map_flags = 0;
    map_flags |= (MEM_RESERVE * map.noreserve);
    DWORD protection_flags = 0;
    protection_flags |= PAGE_READWRITE * (!protection.write && !protection.read);
    protection_flags |= PAGE_READWRITE * (protection.write && protection.read);
    protection_flags |= PAGE_READONLY * (protection.write && !protection.read);
    return (u8*)VirtualAlloc((void*)base, size, map_flags, protection_flags);
#else
    int protection_flags = (protection.read * PROT_READ) | (protection.write * PROT_WRITE) | (protection.execute * PROT_EXEC);
    int map_flags = (map.anon * MAP_ANONYMOUS) | (map.priv * MAP_PRIVATE) | (map.noreserve * MAP_NORESERVE);
    u8* result = (u8*)mmap((void*)base, size, protection_flags, map_flags, -1, 0);
    assert(result != MAP_FAILED);
    return result;
#endif
}

fn void os_commit(void* address, u64 size)
{
#if _WIN32
    VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE);
#else
    int result = mprotect(address, size, PROT_READ | PROT_WRITE);
    assert(result == 0);
#endif
}

fn void vb_generic_ensure_capacity(VirtualBuffer(u8)* vb, u32 item_size, u32 item_count)
{
    u32 old_capacity = vb->capacity;
    u32 wanted_capacity = vb->length + item_count;

    if (old_capacity < wanted_capacity)
    {
        if (old_capacity == 0)
        {
            vb->pointer = os_reserve(0, item_size * UINT32_MAX, (OSReserveProtectionFlags) {}, (OSReserveMapFlags) { .priv = 1, .anon = 1, .noreserve = 1 });
        }

        let_cast(u32, old_page_capacity, align_forward(old_capacity * item_size, minimum_granularity));
        let_cast(u32, new_page_capacity, align_forward(wanted_capacity * item_size, minimum_granularity));

        let(commit_size, new_page_capacity - old_page_capacity);
        void* commit_pointer = vb->pointer + old_page_capacity;

        os_commit(commit_pointer, commit_size);

        let(new_capacity, new_page_capacity / item_size);
        vb->capacity = new_capacity;
    }
}

fn u8* vb_generic_add_assume_capacity(VirtualBuffer(u8)* vb, u32 item_size, u32 item_count)
{
    u32 index = vb->length;
    assert(vb->capacity >= index + item_count);
    vb->length = index + item_count;
    return vb->pointer + (index * item_size);
}

fn u8* vb_generic_add(VirtualBuffer(u8)* vb, u32 item_size, u32 item_count)
{
    vb_generic_ensure_capacity(vb, item_size, item_count);
    return vb_generic_add_assume_capacity(vb, item_size, item_count);
}

fn u8* vb_append_bytes(VirtualBuffer(u8*) vb, Slice(u8) bytes)
{
    let_cast(u32, len, bytes.length);
    vb_generic_ensure_capacity(vb, sizeof(u8), len);
    let(pointer, vb_generic_add_assume_capacity(vb, sizeof(u8), len));
    memcpy(pointer, bytes.pointer, len);
    return pointer;
}

fn u32 vb_copy_string(VirtualBuffer(u8)* buffer, String string)
{
    let(offset, buffer->length);
    let_cast(u32, length, string.length);
    let(pointer, vb_add(buffer, length));
    memcpy(pointer, string.pointer, length);
    return offset;
}

fn u64 vb_copy_string_zero_terminated(VirtualBuffer(u8)* buffer, String string)
{
    assert(string.pointer[string.length] == 0);
    string.length += 1;

    vb_copy_string(buffer, string);

    return string.length;
}

STRUCT(Writer)
{
    VirtualBuffer(u8) buffer;
};

fn void write_line(Writer* writer, String string)
{
    vb_copy_string(&writer->buffer, string);
    *vb_add(&writer->buffer, 1) = '\n';
}

fn void write_head(Writer* writer)
{
    write_line(writer, strlit("<head>"));
    write_line(writer, strlit("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"));
    write_line(writer, strlit("<title>David Gonzalez Martin</title>"));
    write_line(writer, strlit("</head>"));
}

fn void write_body(Writer* writer)
{
    write_line(writer, strlit("<body>"));
    write_line(writer, strlit("<h2>Work</h2>"));
    write_line(writer, strlit("<ul>"));
    write_line(writer, strlit("<li><a href=\"https://github.com/davidgmbb\" target=\"_blank\">Personal GitHub</a></li>"));
    write_line(writer, strlit("<li><a href=\"https://github.com/birth-software\" target=\"_blank\">Birth GitHub</a></li>"));
    write_line(writer, strlit("</ul>"));
    write_line(writer, strlit("<h2>Resume</h2>"));
    write_line(writer, strlit("<ul>"));
    write_line(writer, strlit("<li><a href=\"resume/resume.pdf\" target=\"_blank\">PDF</a></li>"));
    write_line(writer, strlit("</ul>"));

    write_line(writer, strlit("</body>"));
}

fn void html_start(Writer* writer)
{
    write_line(writer, strlit("<!DOCTYPE html>"));
    write_line(writer, strlit("<html lang=\"en\">"));
}

fn void html_end(Writer* writer)
{
    write_line(writer, strlit("</html>"));
}

fn void write_document(Writer* writer)
{
    html_start(writer);
    write_head(writer);
    write_body(writer);
    html_end(writer);
}

fn const char* join_path2(const char* p1, const char* p2)
{
    let(p1_length, strlen(p1));
    let(p2_length, strlen(p2));
    let(length, p1_length + p2_length + 2);

    char* allocation = malloc(length);
    size_t index = 0;
    memcpy(&allocation[index], p1, p1_length);
    index += p1_length;
    allocation[index] = '/';
    index += 1;
    memcpy(&allocation[index], p2, p2_length);
    index += p2_length;
    allocation[index] = 0;

    return allocation;
}

int main()
{
    char* public_directory = getenv("WEB_PUBLIC_DIRECTORY");
    const char* index_html_path = join_path2(public_directory, "index.html");
    int fd = open(index_html_path, O_TRUNC | O_WRONLY | O_CREAT, 0644);
    assert(fd >= 0);
    Writer writer = {};
    write_document(&writer);
    let(result, write(fd, writer.buffer.pointer, writer.buffer.length));
    assert(result == writer.buffer.length);
    close(fd);
}
