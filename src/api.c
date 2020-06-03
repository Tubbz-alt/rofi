/**
 * @file src/api.c
 * @brief ROFI APIs
 *
 * This file implements all the ROFI APIs, including initialization and
 * finalization of the runtime library.
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <rofi_debug.h>
#include <rofi_internal.h>

/**
 * @brief ROFI Initialization function.
 *
 * Calling this function is necessary to initialize ROFI internal
 * state and to start the underlying OFI and runtime layer. Using any other APIs
 * without prior call to rofi_init() may result in failure and unexpected behavior.
 *
 *
 * @return 0 on success, -1 in case of errors
 *
 * \b blocking: yes
 * \b thread-safe: no
 *
 */
int rofi_init(void)
{
	int ret = 0;

	rdesc.status = ROFI_STATUS_NONE;

	DEBUG_MSG("Initilizing ROFI runtime...");

	ret = rofi_init_internal();
	
	if(ret){
		ERR_MSG("Error initializing ROFI library");
		return ret;
	}
	
	DEBUG_MSG("Initialization succesfully completed!");
	rdesc.status = ROFI_STATUS_ACTIVE;	
	
	return ret;
}

/**
 * @brief ROFI Get workgroup size
 *
 * This function return the number of processes (locales) that are part of the
 * job. This fucntion behaves similarly to `MPI_Comm_size()`.`
 *
 * @return The number of processes in the job
 *
 * \b blocking: no
 * \b thread-safe: yes
 *
 */
unsigned int rofi_get_size(void)
{
	assert(rdesc.status == ROFI_STATUS_ACTIVE);
	DEBUG_MSG("Getting number of processes (%u)...", rdesc.nodes);
	return rofi_get_size_internal();
}

/**
 * @brief ROFI Get process ID
 *
 * This function return the ID of the calling process within the job. 
 * This fucntion behaves similarly to `MPI_Comm_rank()`.`
 *
 * @return The process ID within the job
 *
 * \b blocking: no
 * \b thread-safe: yes
 *
 */
unsigned int rofi_get_id(void)
{
	assert(rdesc.status == ROFI_STATUS_ACTIVE);
	DEBUG_MSG("Getting process ID (%u)...", rdesc.nid);
	return rofi_get_id_internal();
}

/**
 * @brief ROFI Finalization
 *
 * This function should be called at the end of a program. This function
 * releases all computing and memory resources acquired by the process and
 * make sure that communcation is properly re-routed. Users should not call
 * additional ROFI calls after this function.
 *
 * @return 0 on success
 *
 * \b blocking: no
 * \b thread-safe: yes
 *
 */
int rofi_finit(void)
{
	assert(rdesc.status == ROFI_STATUS_ACTIVE);
	DEBUG_MSG("Finalizing ROFI runtime...");
	rofi_finit_internal();
	DEBUG_MSG("ROFI runtime shutdown completed.");
	return 0;
}

/**
 * @brief ROFI Asynchronous PUT
 *
 * This fucntion transfer \p size bytes starting at address \p src in the current
 * process virtual address space to process \p id at address \p dst at destination
 * asynchronously. Users are supposed to either check that the transfer has been
 * completed or issue a `rofi_wait()`. It is expected that an heap has been
 * establisehd. Buffers should not be re-used before the 
 * transfer is completed.
 *
 * @param[out] dst address at destiantion (output at destination, not used at source)
 * @param[in]  src address at source (input at source, not used at destination)
 * @param[in]  size size of the entire buffer in bytes (no padding)
 * @param[in]  id the ID of the remote node
 * @param[in]  flags (not used at this time)
 * @return 0 on success
 *
 * \b blocking: no
 * \b thread-safe: yes
 *
 */
int rofi_put(void* dst, const void* src, size_t size, unsigned int id, unsigned long flags)
{
	assert(rdesc.status == ROFI_STATUS_ACTIVE);

	if(dst == NULL || src == NULL || size == 0 || id >= rdesc.nodes){
		ERR_MSG("Invalide argument.");
		return -1;
	}

	DEBUG_MSG("PUT ASYNC src %p dst %p size %lu flags 0x%lx",
		  src, dst, size, flags);
	
	return rofi_put_internal(dst,src,size,id, flags | ROFI_ASYNC);
}

/**
 * @brief ROFI Synchronous PUT
 *
 * Similar to `rofi_put()` but blocks until the transfer has completed. Users are
 * free to re-used buffers after returning from this call.
 *
* @param[out] dst address at destiantion (output at destination, not used at source)
 * @param[in]  src address at source (input at source, not used at destination)
 * @param[in] size size of the entire buffer in bytes (no padding)
 * @param[in] id the ID of the remote node
 * @param[in] flags (dot used at this time.
 * @return 0 on success
 *
 * \b blocking: yes
 * \b thread-safe: yes
 *
 */
int rofi_iput(void* dst, const void* src, size_t size, unsigned int id, unsigned long flags)
{
	assert(rdesc.status == ROFI_STATUS_ACTIVE);

	if(dst == NULL || src == NULL || size == 0 || id >= rdesc.nodes){
		ERR_MSG("Invalide argument.");
		return -1;
	}

	DEBUG_MSG("PUT SYNC src %p dst %p size %lu flags 0x%lx",
		  src, dst, size, flags);
	
	return rofi_put_internal(dst,src,size,id, flags | ROFI_SYNC);
}

/**
 * @brief ROFI Asynchronous GET
 *
 * This fucntion transfer \p size bytes starting at address \p src in the remote
 * process virtual address space \p id to the current process at address \p dst
 * asynchronously. Users are supposed to either check that the transfer has been
 * completed or issue a `rofi_wait()`. It is expected that an heap has been
 * establisehd. Buffers should not be re-used before the 
 * transfer is completed.
 *
 * @param[out] dst address at destiantion (output at destination, not used at source)
 * @param[in]  src address at source (input at source, not used at destination)
 * @param[in] size size of the entire buffer in bytes (no padding)
 * @param[in] id the ID of the remote node
 * @param[in] flags (not used at this time)
 * @return 0 on success
 *
 * \b blocking: no
 * \b thread-safe: yes
 *
 */
int rofi_get(void* dst, const void* src, size_t size, unsigned int id, unsigned long flags)
{
	assert(rdesc.status == ROFI_STATUS_ACTIVE);

	if(dst == NULL || src == NULL || size == 0 || id >= rdesc.nodes){
		ERR_MSG("Invalide argument.");
		return -1;
	}

	DEBUG_MSG("GET src %p dst %p size %lu flags 0x%lx",
		  src, dst, size, flags);
	
	return rofi_get_internal(dst,src,size,id,flags | ROFI_ASYNC);
}

/**
 * @brief ROFI Synchronous PUT
 *
 * Similar to `rofi_put()` but blocks until the transfer has completed. Users are
 * free to re-used buffers after returning from this call.
 *
 * @param[out] dst address at destiantion (output at destination, not used at source)
 * @param[in]  src address at source (input at source, not used at destination)
 * @param[in] size size of the entire buffer in bytes (no padding)
 * @param[in] id the ID of the remote node
 * @param[in] flags (not used at this time)
 * @return 0 on success
 *
 * \b blocking: no
 * \b thread-safe: yes
 *
 */
int rofi_iget(void* dst, const void* src, size_t size, unsigned int id, unsigned long flags)
{
	assert(rdesc.status == ROFI_STATUS_ACTIVE);

	if(dst == NULL || src == NULL || size == 0 || id >= rdesc.nodes){
		ERR_MSG("Invalide argument.");
		return -1;
	}

	DEBUG_MSG("GET src %p dst %p size %lu flags 0x%lx",
		  src, dst, size, flags);
	
	return rofi_get_internal(dst,src,size,id,flags | ROFI_SYNC);
}

/**
 * @brief ROFI Global Barrier
 *
 * This function blocks until all processes in the job have called `rofi_barrier()`. All
 * processes involved in the barrier will be released when the last process enters the 
 * barrier.
 *
 * \b blocking: no
 * \b thread-safe: yes
 *
 */
void rofi_barrier(void)
{
	assert(rdesc.status == ROFI_STATUS_ACTIVE);
	DEBUG_MSG("Process %u/%u entering barrier...", rdesc.nid, rdesc.nodes);
	rt_barrier();
	DEBUG_MSG("Process %u/%u leaving barrier...", rdesc.nid, rdesc.nodes);
}

/**
 * @brief ROFI Memory Region allocation
 *
 * This function allocates a memory region of \p size bytes and registers it to be accessible
 * remotely from other compute nodes (RDMA). If all processes in the job call this function,
 * the application has effectively allocated a synmmetric heap that can be
 * accessed remotely. Currently, only one memory region can be allocated at any given time. 
 * An allocated memory region can be deallocated using `rofi_release()`.
 *
 * @param[in] size size of the memory region
 * @param[in] flags (not used at this time)
 * @param[out] addr initial address of the allocated memory region
 * @return 0 on success, -1 on failure
 *
 * \b blocking: potentially (memory allocation)
 * \b thread-safe: yes
 *
 */
int rofi_alloc(size_t size, unsigned long flags, void** addr)
{
	DEBUG_MSG("ALLOC size %lu flags 0x%lx",
		  size, flags);

	if(!size){
		ERR_MSG("Invalid size (%lu)",size);
		goto err;
	}
	
	*addr = rofi_alloc_internal(size, flags);
	if(*addr == NULL)
		goto err;

	DEBUG_MSG("\tAllocated symmetric heap of size %lu at %p", size, *addr);
	return 0;

 err:
	return -1;
}

/**
 * @brief ROFI Memory Region release
 *
 * This function releases a remote-accessible memory region previously allocated through `rofi_alloc()`.
 * The function will fail if no memory region has been allocated yet.
 *
 * @return 0 on success, -1 on failure
 *
 * \b blocking: no
 * \b thread-safe: yes
 *
 */
int rofi_release(void)
{
	DEBUG_MSG("Releaseing symmetric heap...");
	return rofi_release_internal();
}

/**
 * @brief ROFI Wait
 *
 * This function blocks until outstanding remote memory operations have completed. The function is
 * meant to be used in conjuction with `rofi_put()` and `rofi_get()` API families to guarantee that 
 * all transfers are completed. Note that users can employ different application-specific methods to
 * understand that a particular transfer is completed.
 *
 * \b blocking: yes
 * \b thread-safe: yes
 *
 */
int rofi_wait(void)
{
	DEBUG_MSG("Waiting for asynchronous messages...");
	return rofi_wait_internal();	
}