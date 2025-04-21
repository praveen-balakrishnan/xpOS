/**
    Copyright 2023-2025 Praveen Balakrishnan

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    xpOS v1.0
*/

#ifndef XPOS_COMMON_REFERENCECOUNTING_H
#define XPOS_COMMON_REFERENCECOUNTING_H

namespace Common
{
    /**
     * Automatically reference counts a shared resource.
     */
    template <class T>
    class AutomaticReferenceCountable
    {
    public:
        AutomaticReferenceCountable(T resource)
        {
            // Allocate a new control block with the resource.
            m_controlBlock = new AutomaticReferenceCountableControlBlock(std::move(resource));
            //m_controlBlock->m_resource = resource;
            m_controlBlock->m_refCount++;
        }

        AutomaticReferenceCountable(const AutomaticReferenceCountable& sharedResource)
        {
            // We are constructing with another shared resource, so copy the control block and increment refcount.
            m_controlBlock = sharedResource.m_controlBlock;
            m_controlBlock->m_refCount++;
        }

        AutomaticReferenceCountable& operator=(AutomaticReferenceCountable sharedResource)
        {
            // We are constructing with another shared resource, so copy the control block and increment refcount.
            auto tempControlBlock = sharedResource.m_controlBlock;
            sharedResource.m_controlBlock = this->m_controlBlock;
            this->m_controlBlock = tempControlBlock;
            return *this;
        }

        AutomaticReferenceCountable(AutomaticReferenceCountable&& sharedResource)
        {
            // Using move semantics, we steal the control block from the old shared resource.
            m_controlBlock = sharedResource.m_controlBlock;
            sharedResource.m_controlBlock = nullptr;
        }

        AutomaticReferenceCountable& operator=(AutomaticReferenceCountable&& sharedResource)
        {
            // Using move semantics, we steal the control block from the old shared resource.
            m_controlBlock = sharedResource.m_controlBlock;
            sharedResource.m_controlBlock = nullptr;
            return *this;
        }

        friend void swap(AutomaticReferenceCountable& a, AutomaticReferenceCountable& b)
        {
            using std::swap;
            swap(a.m_controlBlock, b.m_controlBlock);
        }

        T& operator*()
        {
            return m_controlBlock->m_resource;
        }

        ~AutomaticReferenceCountable()
        {
            if (!m_controlBlock)
                return;
            m_controlBlock->m_refCount--;

            if (m_controlBlock->m_refCount == 0)
                delete m_controlBlock;
        }

    private:
        class AutomaticReferenceCountableControlBlock
        {
            friend class AutomaticReferenceCountable<T>;
            T m_resource;
            std::size_t m_refCount = 0;
            AutomaticReferenceCountableControlBlock(T resource)
                : m_resource(std::move(resource))
            {}
        };
        AutomaticReferenceCountableControlBlock* m_controlBlock = nullptr;
    };

}

#endif