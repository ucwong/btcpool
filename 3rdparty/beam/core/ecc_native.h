// Copyright 2018 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include "ecc.h"
#include <assert.h>

#define USE_BASIC_CONFIG

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wunused-function"
#else
#	pragma warning (push, 0) // suppress warnings from secp256k1
#	pragma warning (disable: 4706) // assignment within conditional expression
#endif

#include "secp256k1-zkp/src/basic-config.h"
#include "secp256k1.h"
#include "secp256k1-zkp/src/hash.h"

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#	pragma GCC diagnostic pop
#else
#	pragma warning (default: 4706)
#	pragma warning (pop)
#endif

namespace ECC
{
	class Hash::Processor
		:private secp256k1_sha256_t
	{
		bool m_bInitialized;

		void Write(const void*, uint32_t);
		void Write(const beam::Blob&);

		void Finalize(Value&);

	public:
		Processor();
		~Processor();

		void Reset();

		template <typename T>
		Processor& operator << (const T& t) { Write(t); return *this; }

		void operator >> (Value& hv) { Finalize(hv); }
	};
}
