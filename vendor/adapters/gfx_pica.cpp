/* Keep the vendored backend intact; add a fault-only frame cancellation path. */
#include "../eog-pica/src/gfx_pica.cpp"
extern "C" void pd_native_graphics_abort(void) {
  if(in_frame) {
    /* Drop pending CPU batches; already submitted GPU commands remain valid. */
    pending_draw.clear();vertex_count=0;world_index_count=0;
    C3D_FrameEnd(0);in_frame=false;
  }
  native_graphics_shutdown();
}
