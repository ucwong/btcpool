/*
 The MIT License (MIT)

 Copyright (c) [2016] [BTC.COM]

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */
#ifndef STRATUM_H_
#define STRATUM_H_

#include <functional>

#include "Common.h"
#include "Utils.h"
#include "Network.h"

// default worker name
#define DEFAULT_WORKER_NAME "__default__"

inline uint32_t jobId2Time(uint64_t jobId) {
  return (uint32_t)((jobId >> 32) & 0x00000000FFFFFFFFULL);
}

string filterWorkerName(const string &workerName);

inline string filterWorkerName(const char *workerName) {
  return filterWorkerName(std::string(workerName));
}

//////////////////////////////// StratumError ////////////////////////////////
class StratumStatus {
public:
  enum {
    // make ACCEPT and SOLVED be two singular value,
    // so code bug is unlikely to make false ACCEPT shares

    // share reached the job target (but may not reached the network target)
    ACCEPT = 1798084231, // bin(01101011 00101100 10010110 10000111)

    // share reached the job target but the job is stale
    // if uncle block is allowed in the chain, share can be accept as this
    // status
    ACCEPT_STALE = 950395421, // bin(00111000 10100101 11100010 00011101)

    // share reached the network target
    SOLVED = 1422486894, // bin(‭01010100 11001001 01101101 01101110‬)

    // share reached the network target but the job is stale
    // if uncle block is allowed in the chain, share can be accept as this
    // status
    SOLVED_STALE = 1713984938, // bin(01100110 00101001 01010101 10101010)

    // share reached the network target but the correctness is not verified
    SOLVED_PRELIMINARY =
        1835617709, // // bin(01101101 01101001 01001101 10101101)

    REJECT_NO_REASON = 0,

    JOB_NOT_FOUND_OR_STALE = 21,
    DUPLICATE_SHARE = 22,
    LOW_DIFFICULTY = 23,
    UNAUTHORIZED = 24,
    NOT_SUBSCRIBED = 25,

    ILLEGAL_METHOD = 26,
    ILLEGAL_PARARMS = 27,
    IP_BANNED = 28,
    INVALID_USERNAME = 29,
    INTERNAL_ERROR = 30,
    TIME_TOO_OLD = 31,
    TIME_TOO_NEW = 32,
    ILLEGAL_VERMASK = 33,

    INVALID_SOLUTION = 34,
    WRONG_NONCE_PREFIX = 35,

    JOB_NOT_FOUND = 36,
    STALE_SHARE = 37,

#ifdef WORK_WITH_STRATUM_SWITCHER
    CLIENT_IS_NOT_SWITCHER = 400,
#endif

    UNKNOWN = 2147483647 // bin(01111111 11111111 11111111 11111111)
  };

  static const char *toString(int err);

  inline static bool isAccepted(int status) {
    return (status == ACCEPT) || (status == ACCEPT_STALE) ||
        (status == SOLVED) || (status == SOLVED_STALE);
  }

  inline static bool isAcceptedStale(int status) {
    return (status == ACCEPT_STALE) || (status == SOLVED_STALE);
  }

  inline static bool isRejectedStale(int status) {
    return (status == JOB_NOT_FOUND_OR_STALE) || (status == STALE_SHARE);
  }

  inline static bool isAnyStale(int status) {
    return isAcceptedStale(status) || isRejectedStale(status);
  }

  inline static bool isSolved(int status) {
    return (status == SOLVED) || (status == SOLVED_STALE) ||
        (status == SOLVED_PRELIMINARY);
  }
};

//////////////////////////////// StratumWorker ////////////////////////////////
class StratumWorkerPlain {
public:
  int32_t userId_;
  int64_t workerHashId_;

  string fullName_;
  string userName_;
  string workerName_;
};

class StratumWorker {
public:
  std::atomic<size_t> chainId_;
  vector<int32_t> userIds_;
  int64_t workerHashId_; // substr(0, 8, HASH(wokerName))

  string fullName_; // fullName = username.workername
  string userName_;
  string workerName_; // workername, max is: 20

  void resetNames();

public:
  StratumWorker(const size_t chainSize);

  void setChainIdAndUserId(const size_t chainId, const int32_t userId);
  void setNames(
      const string &fullName,
      std::function<void(string &)> userNormalizer,
      bool singleUserMode = false,
      const string &singleUserName = "",
      bool extralength = false);

  int32_t userId() const { return userIds_[chainId_]; }
  int32_t userId(const size_t chainId) const { return userIds_[chainId]; }

  static string getUserName(const string &fullName);
  static string getWorkerName(const string &fullName);
  static int64_t calcWorkerId(const string &workerName);
};

////////////////////////////////// StratumJob //////////////////////////////////
//
// Stratum Job
//
// https://slushpool.com/help/#!/manual/stratum-protocol
//
// "mining.notify"
//
// job_id   - ID of the job. Use this ID while submitting share generated
//             from this job.
// prevhash - Hash of previous block.
// coinb1   - Initial part of coinbase transaction.
// coinb2   - Final part of coinbase transaction.
// merkle_branch - List of hashes, will be used for calculation of merkle root.
//                 This is not a list of all transactions, it only contains
//                 prepared hashes of steps of merkle tree algorithm.
// version    - Bitcoin block version.
// nbits      - Encoded current network difficulty
// ntime      - Current ntime
// clean_jobs - When true, server indicates that submitting shares from previous
//              jobs don't have a sense and such shares will be rejected. When
//              this flag is set, miner should also drop all previous jobs,
//              so job_ids can be eventually rotated.
//
//
class StratumJob {
public:
  // jobId: timestamp + gbtHash, hex string, we need to make sure jobId is
  // unique in a some time, jobId can convert to uint64_t
  uint64_t jobId_;

protected:
  StratumJob(); //  protected so cannot create it.
public:
  virtual ~StratumJob();

  virtual string serializeToJson() const = 0;
  virtual bool unserializeFromJson(const char *s, size_t len) = 0;
  virtual uint32_t jobTime() const { return jobId2Time(jobId_); }
  virtual uint64_t height() const = 0;
};

struct LocalJob {
  size_t chainId_;
  uint64_t jobId_;

  LocalJob(size_t chainId, uint64_t jobId)
    : chainId_(chainId)
    , jobId_(jobId) {}

  bool operator==(uint64_t jobId) const { return jobId_ == jobId; }
};

template <typename LocalShareType>
struct LocalJobBase : public LocalJob {
  std::set<LocalShareType> submitShares_;

  LocalJobBase(size_t chainId, uint64_t jobId)
    : LocalJob(chainId, jobId) {}

  bool addLocalShare(const LocalShareType &localShare) {
    return submitShares_.insert(localShare).second;
  }
};

namespace sharebase {

template <typename ShareMsg>
class Serializable : public ShareMsg {
public:
  bool SerializeToStringWithVersion(string &data) const {
    data.resize(sizeof(uint32_t));
    *reinterpret_cast<uint32_t *>(&data[0]) = this->version();
    return this->AppendToString(&data);
  }
};

template <typename Derived, typename ShareMsg>
class Unserializable : public Serializable<ShareMsg> {
public:
  bool UnserializeWithVersion(const uint8_t *data, uint32_t size) {
    if (nullptr == data || size <= sizeof(size)) {
      return false;
    }

    auto version = *reinterpret_cast<const uint32_t *>(data);
    return version == Derived::CURRENT_VERSION &&
        this->ParseFromArray(data + sizeof(uint32_t), size - sizeof(uint32_t));
  }
};

} // namespace sharebase

#endif
