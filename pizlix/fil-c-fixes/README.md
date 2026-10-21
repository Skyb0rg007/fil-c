# Sprint 5 runtime fixes (to mirror into pizlix/ repo scripts after reboot to Ubuntu)

Live-fix history: console login was broken by (a) pam_elogind.so exporting no symbols,
(b) /run/user created 1777 by mountvirtfs. Session launch was broken by a chain of
Fil-C incompatibilities in mesa/cogl/nir. All fixes below are applied to the LIVE
system; each must be mirrored into the corresponding pizlix/build_postlc*_chroot_*.sh
script so a full rebuild reproduces them.

1. pizlix/build_postlc7_chroot_project_elogind.sh
   After `tar -xf pizlonated-elogind.tar.gz`, patch src/login/pam_elogind.sym:
   global section must also list pizlonated_pam_sm_close_session and
   pizlonated_pam_sm_open_session (Fil-C exports pizlonated_<name> aliases and
   libpam's dlsym resolves those; the version script's "local: *" was demoting
   them -> module had zero exports -> login failed with "Module is unknown").

2. /etc/rc.d/init.d/mountvirtfs (add to setup script):
   sed -i 's|mkdir -m 1777 /run/user|mkdir -m 755 /run/user|'
   (elogind refuses to create sessions while /run/user is 1777).

3. pizlix/build_postlc4_chroot_project_mesa.sh (after tar -xf, before build):
   a. src/gallium/targets/dri/dri.sym.in: add `pizlonated___driDriverGetExtensions*;`
      to global (same reason as pam_elogind.sym).
   b. src/gallium/targets/dri/target.c: add
      `__driDriverGetExtensions_bochs_drm` returning dri_swrast_kms_driver_extensions
      (QEMU std VGA = bochs-drm kernel driver; mesa loader uses the kernel name).
   c. src/gallium/targets/dri/meson.build: install 'bochs-drm_dri.so' alongside
      kms_swrast_dri.so (list after `with_gallium_softpipe and with_gallium_drisw_kms`).
   d. src/util/u_cpu_detect.c: xgetbv() -> return 0 (raw XGETBV inline asm is
      rejected by Fil-C's validator; llvmpipe then avoids AVX paths).
   e. src/util/sparse_array.{c,h}: node handles were tag-bit-carrying uintptr_t
      (pointer | level); Fil-C rejects the int->pointer cast. Nodes now carry the
      level in an 8-byte header before the payload; handles are real pointers.
   f. include/EGL/eglplatform.h __unix__ branch: EGLNativeDisplayType/PixmapType/
      WindowType are khronos_uintptr_t upstream -> every EGL entry point taking a
      native window lost pointer object metadata. Changed all three to void*.
      REBUILD ORDER NOTE: mesa is built in postlc4 before gtk4 (postlc4), libepoxy
      (postlc6) and mutter (postlc7), so all consumers get the fixed header.
   g. src/egl/drivers/dri2/platform_drm.c: read the surface format via
      gbm_surface_get_format(surface) instead of surface->v0.format (direct
      cross-DSO deref of a libgbm allocation loses object metadata).
   h. src/gbm/main/gbm.{h,c}: added gbm_surface_get_format().
   i. src/egl/main/egldevice.c: the software device no longer advertises
      EGL_EXT_device_drm_render_node (it has no drmDevice; mutter probes that
      extension and then queries the render node -> NULL deref).
   j. src/egl/main/egldevice.{h,c} + eglapi.c: EGL_DEVICE_EXT handles returned by
      eglQueryDisplayAttrib are EGLAttrib integers; encode them in a zptrtable
      (_eglEncodeDeviceHandle) and decode in _eglLookupDevice
      (_eglDecodeDeviceHandle) so the round trip preserves object metadata.
   k. src/mesa/main/queryobj.c: get_query_object/store_query_result take the
      parameter buffer as void* (was intptr_t round trip).
   l. src/compiler/glsl_types.c: glsl_type name_id is a pointer stored as an
      integer; glsl_get_type_name() now decodes through a zptrtable
      (filc_encode_type_name/filc_decode_type_name; builtin-name offsets skip it).
   m. src/compiler/nir/nir.h + nir_validate.c (+ all nir files): nir_src._parent
      was a tag-bit-carrying uintptr_t. Now a real `void *_parent` pointer plus
      `uintptr_t _parent_flags` (NIR_SRC_PARENT_IS_IF / SRC_TAG_SEEN bits).
      nir.h accessors rewritten; direct accesses in *.c updated.

4. pizlix/build_postlc7_chroot_project_mutter.sh (after tar -xf):
   a. cogl/cogl/cogl-cpu-caps.c: xgetbv() -> return 0 (same XGETBV issue as mesa).
   b. cogl/cogl/cogl-bitmap-conversion.c: disable COGL_USE_PREMULT_SSE2
      (#if 0 && defined(__SSE2__)...) - the SSE2 block is inline asm with pointer
      arguments, unsupported by Fil-C.

5. Post-fix rebuilds done on the live system: elogind, mesa, mutter, gtk4,
   libepoxy (libepoxy's generated EGL wrappers were built before the header fix;
   gtk4 uses epoxy's global entry points). gtk3 also predates the header fix and
   should be rebuilt the same way when a gtk3 EGL consumer is needed.

6. Runtime env: the mutter session launcher (/usr/bin/pizlix-mutter, installed from
   pizlix/etc/gnome/pizlix-mutter.sh) exports GBM_ALWAYS_SOFTWARE=true (display-only
   bochs-drm must use kms_swrast) and removes stale wayland sockets before starting
   mutter. Both live in the repo file - keep them in sync.
