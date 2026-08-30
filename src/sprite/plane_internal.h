#ifndef PLANE_INTERNAL_H
#define PLANE_INTERNAL_H

namespace PLANE_INTERNAL
{

#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline, no_sanitize("vptr")))
#endif
void RetailExactEmptyCheck(void* p_object);

} // namespace PLANE_INTERNAL

#endif
