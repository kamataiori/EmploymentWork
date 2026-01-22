#include "Collider.h"
#include <atomic>

static std::atomic<uint32_t> gColliderSerial{ 1 };

Collider::Collider()
{
    instanceId_ = gColliderSerial.fetch_add(1);
}

//void Collider::OnCollision()
//{
//}
