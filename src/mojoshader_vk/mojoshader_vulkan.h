/* MojoShader's Vulkan interface. MojoShader removed it in January 2025 (icculus/mojoshader 40e61bf, d5b2957);
 * it is kept here, unchanged, for FNA3D's Vulkan driver. Taken from icculus/mojoshader 40e61bf^. */
#ifndef MOJOSHADER_VULKAN_H
#define MOJOSHADER_VULKAN_H
#include "mojoshader.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Vulkan interface */

/* Avoid including vulkan.h, don't define handles if it's already included */
#ifdef VULKAN_H_
#define NO_MOJOSHADER_VULKAN_TYPEDEFS
#endif
#ifndef NO_MOJOSHADER_VULKAN_TYPEDEFS
#ifdef _WIN32
#define VKAPI_CALL __stdcall
#define VKAPI_PTR VKAPI_CALL
#else
#define VKAPI_CALL
#define VKAPI_PTR
#endif
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef unsigned long long object;
#endif

VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkBuffer)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkShaderModule)

#endif /* !NO_MOJOSHADER_VULKAN_TYPEDEFS */

typedef void (VKAPI_PTR *PFN_MOJOSHADER_vkVoidFunction)(void);
typedef PFN_MOJOSHADER_vkVoidFunction (VKAPI_PTR *PFN_MOJOSHADER_vkGetDeviceProcAddr)(
	VkDevice device,
	const char* pName
);
typedef PFN_MOJOSHADER_vkVoidFunction (VKAPI_PTR *PFN_MOJOSHADER_vkGetInstanceProcAddr)(
	VkInstance instance,
	const char* pName
);

typedef struct MOJOSHADER_vkContext MOJOSHADER_vkContext;
typedef struct MOJOSHADER_vkShader MOJOSHADER_vkShader;
typedef struct MOJOSHADER_vkProgram MOJOSHADER_vkProgram;

/*
 * Prepares a context to manage Vulkan shaders.
 *
 * Don't call this unless you know for sure that you need it.
 *
 * You must call this after creating VkDevice and VkInstance.
 *
 * (instance) refers to VkInstance, cast to void*.
 *
 * (device) refers to VkDevice, cast to void*.
 *
 * (frames_in_flight) refers to the maximum number of frames that can be
 * processed simultaneously.
 *
 * (lookup) refers to PFN_vkGetDeviceProcAddr, a function pointer that
 * is used to dynamically link required Vulkan functions.
 *
 * You must pass in the graphics queue family index and the memory type index
 * you will be using with your Vulkan instance.
 *
 * You can only have one MOJOSHADER_vkContext per actual Vulkan context, or
 *  undefined behaviour will result.
 *
 * As MojoShader requires some memory to be allocated, you may provide a
 *  custom allocator to this function, which will be used to allocate/free
 *  memory. They function just like malloc() and free(). We do not use
 *  realloc(). If you don't care, pass NULL in for the allocator functions.
 *  If your allocator needs instance-specific data, you may supply it with the
 *  (malloc_d) parameter. This pointer is passed as-is to your (m) and (f)
 *  functions.
 *
 * The context created by this function will automatically become the current
 *  context. No further action is needed by the caller.
 *
 * Returns 0 on success or -1 on failure.
 */

DECLSPEC MOJOSHADER_vkContext *MOJOSHADER_vkCreateContext(VkInstance *instance,
                                                          VkPhysicalDevice *physical_device,
                                                          VkDevice *logical_device,
                                                          PFN_MOJOSHADER_vkGetInstanceProcAddr instance_lookup,
                                                          PFN_MOJOSHADER_vkGetDeviceProcAddr lookup,
                                                          unsigned int graphics_queue_family_index,
                                                          unsigned int max_uniform_buffer_range,
                                                          unsigned int min_uniform_buffer_offset_alignment,
                                                          MOJOSHADER_malloc m, MOJOSHADER_free f,
                                                          void *malloc_d);

/*
 * Get any error state we might have picked up.
 *
 * Returns a human-readable string. This string is for debugging purposes, and
 *  not guaranteed to be localized, coherent, or user-friendly in any way.
 *  It's for programmers!
 *
 * The latest error may remain between calls. New errors replace any existing
 *  error. Don't check this string for a sign that an error happened, check
 *  return codes instead and use this for explanation when debugging.
 *
 * Do not free the returned string: it's a pointer to a static internal
 *  buffer. Do not keep the pointer around, either, as it's likely to become
 *  invalid as soon as you call into MojoShader again.
 *
 * This call does NOT require a valid MOJOSHADER_vkContext to have been made
 *  current. The error buffer is shared between contexts, so you can get
 *  error results from a failed MOJOSHADER_vkCreateContext().
 */
DECLSPEC const char *MOJOSHADER_vkGetError(MOJOSHADER_vkContext *context);

/*
 * Deinitialize MojoShader's Vulkan shader management.
 *
 * You must call this once, while your Vulkan context (not MojoShader context) is
 *  still current, if you previously had a successful call to
 *  MOJOSHADER_vkCreateContext(). This should be the last MOJOSHADER_vk*
 *  function you call until you've prepared a context again.
 *
 * This will clean up resources previously allocated, and may call into Vulkan.
 *
 * This will not clean up shaders and programs you created! Please call
 *  MOJOSHADER_vkDeleteShader() and MOJOSHADER_vkDeleteProgram() to clean
 *  those up before calling this function!
 *
 * This function destroys the MOJOSHADER_vkContext you pass it. If it's the
 *  current context, then no context will be current upon return.
 */
DECLSPEC void MOJOSHADER_vkDestroyContext(MOJOSHADER_vkContext *ctx);

/*
 * Compile a buffer of Direct3D shader bytecode into a Vulkan shader module.
 *
 *   (tokenbuf) is a buffer of Direct3D shader bytecode.
 *   (bufsize) is the size, in bytes, of the bytecode buffer.
 *   (swiz), (swizcount), (smap), and (smapcount) are passed to
 *   MOJOSHADER_parse() unmolested.
 *
 * Returns NULL on error, or a shader handle on success.
 *
 * Compiled shaders from this function may not be shared between contexts.
 */
DECLSPEC MOJOSHADER_vkShader *MOJOSHADER_vkCompileShader(MOJOSHADER_vkContext *context,
                                                         const char *mainfn,
                                                         const unsigned char *tokenbuf,
                                                         const unsigned int bufsize,
                                                         const MOJOSHADER_swizzle *swiz,
                                                         const unsigned int swizcount,
                                                         const MOJOSHADER_samplerMap *smap,
                                                         const unsigned int smapcount);

/*
 * Increments a shader's internal refcount.
 *
 * To decrement the refcount, call MOJOSHADER_vkDeleteShader().
 */
DECLSPEC void MOJOSHADER_vkShaderAddRef(MOJOSHADER_vkShader *shader);

/*
 * Decrements a shader's internal refcount, and deletes if the refcount is zero.
 *
 * To increment the refcount, call MOJOSHADER_vkShaderAddRef().
 */
DECLSPEC void MOJOSHADER_vkDeleteShader(MOJOSHADER_vkContext *context,
                                        MOJOSHADER_vkShader *shader);

/*
 * Returns a shader's internal refcount.
 */
DECLSPEC unsigned int MOJOSHADER_vkGetShaderRefCount(MOJOSHADER_vkShader *shader);

/*
 * Get the MOJOSHADER_parseData structure that was produced from the
 *  call to MOJOSHADER_vkCompileShader().
 *
 * This data is read-only, and you should NOT attempt to free it. This
 *  pointer remains valid until the shader is deleted.
 */
DECLSPEC const MOJOSHADER_parseData *MOJOSHADER_vkGetShaderParseData(MOJOSHADER_vkShader *shader);

/*
 * Link a vertex and pixel shader into a working Vulkan shader program.
 *  (vshader) or (pshader) can NOT be NULL, unlike OpenGL.
 *
 * You can reuse shaders in various combinations across
 *  multiple programs, by relinking different pairs.
 *
 * It is illegal to give a vertex shader for (pshader) or a pixel shader
 *  for (vshader).
 *
 * Once you have successfully linked a program, you may render with it.
 *
 * Returns NULL on error, or a program handle on success.
 */
DECLSPEC MOJOSHADER_vkProgram *MOJOSHADER_vkLinkProgram(MOJOSHADER_vkContext *context,
                                                        MOJOSHADER_vkShader *vshader,
                                                        MOJOSHADER_vkShader *pshader);

/*
 * This binds the program to the active context, and does nothing particularly
 * special until you start working with uniform buffers or shader modules.
 *
 * After binding a program, you should update any uniforms you care about
 *  with MOJOSHADER_vkMapUniformBufferMemory() (etc), set any vertex arrays
 *  using MOJOSHADER_vkGetVertexAttribLocation(), and finally call
 *  MOJOSHADER_vkGetShaderModules() to get the final modules. Then you may
 *  begin building your pipeline state objects.
 */
DECLSPEC void MOJOSHADER_vkBindProgram(MOJOSHADER_vkContext *context,
                                       MOJOSHADER_vkProgram *program);

/*
 * Free the resources of a linked program. This will delete the shader modules
 *  and free memory.
 *
 * If the program is currently bound by MOJOSHADER_vkBindProgram(), it will
 *  be deleted as soon as it becomes unbound.
 */
DECLSPEC void MOJOSHADER_vkDeleteProgram(MOJOSHADER_vkContext *context,
                                         MOJOSHADER_vkProgram *program);

/*
 * This "binds" individual shaders, which effectively means the context
 *  will store these shaders for later retrieval. No actual binding or
 *  pipeline creation is performed.
 *
 * This function is only for convenience, specifically for compatibility
 *  with the effects API.
 */
DECLSPEC void MOJOSHADER_vkBindShaders(MOJOSHADER_vkContext *context,
                                       MOJOSHADER_vkShader *vshader,
                                       MOJOSHADER_vkShader *pshader);

/*
 * This queries for the shaders currently bound to the active context.
 *
 * This function is only for convenience, specifically for compatibility
 *  with the effects API.
 */
DECLSPEC void MOJOSHADER_vkGetBoundShaders(MOJOSHADER_vkContext *context,
                                           MOJOSHADER_vkShader **vshader,
                                           MOJOSHADER_vkShader **pshader);

/*
 * Fills register pointers with pointers that are directly used to push uniform
 *  data to the Vulkan shader context.
 *
 * This function is really just for the effects API, you should NOT be using
 *  this unless you know every single line of MojoShader from memory.
 */
DECLSPEC void MOJOSHADER_vkMapUniformBufferMemory(MOJOSHADER_vkContext *context,
                                                  float **vsf, int **vsi, unsigned char **vsb,
                                                  float **psf, int **psi, unsigned char **psb);

/*
 * Tells the context that you are done with the memory mapped by
 *  MOJOSHADER_vkMapUniformBufferMemory().
 */
DECLSPEC void MOJOSHADER_vkUnmapUniformBufferMemory(MOJOSHADER_vkContext *context);

/*
 * This queries for the uniform buffer, byte offset and byte size for each of the
 *  currently bound shaders.
 *
 * This function is only for convenience, specifically for compatibility with
 *  the effects API.
 */
DECLSPEC void MOJOSHADER_vkGetUniformBuffers(MOJOSHADER_vkContext *context,
                                             VkBuffer *vbuf,
                                             unsigned long long *voff,
                                             unsigned long long *vsize,
                                             VkBuffer *pbuf,
                                             unsigned long long *poff,
                                             unsigned long long *psize);

/*
 * Prepares uniform buffers for reuse.
 *
 * Always call this after submitting the final command buffer for a frame!
 */
DECLSPEC void MOJOSHADER_vkEndFrame(MOJOSHADER_vkContext *context);

/*
 * Return the location of a vertex attribute for the given shader.
 *
 * (usage) and (index) map to Direct3D vertex declaration values: COLOR1 would
 *  be MOJOSHADER_USAGE_COLOR and 1.
 *
 * The return value is the index of the attribute to be used to create
 *  a VkVertexInputAttributeDescription, or -1 if the stream is not used.
 */
DECLSPEC int MOJOSHADER_vkGetVertexAttribLocation(MOJOSHADER_vkShader *vert,
                                                  MOJOSHADER_usage usage,
                                                  int index);

/*
 * Get the VkShaderModules from the currently bound shader program.
 */
DECLSPEC void MOJOSHADER_vkGetShaderModules(MOJOSHADER_vkContext *context,
                                            VkShaderModule *vmodule,
                                            VkShaderModule *pmodule);

#ifdef __cplusplus
}
#endif
#endif /* MOJOSHADER_VULKAN_H */
