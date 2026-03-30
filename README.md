# ProcademyProject

C++ 기반 서버 개발을 학습하면서 구현한 프로젝트들을 정리한 저장소입니다.

\---

## 바로가기

### 📌

* [GameServer](./4Cource/GameServer)
* [LoginServer](./4Cource/LoginServer/LoginServer)
* [Single ChattingServer](./4Cource/ChattingServer/NetServer_v4.2)
* [Multi ChattingServer](./4Cource/ChattingServer/NetServer_v3.0_multiThread)
* [MonitoringServer](./4Cource/MonitoringServer/MonitoringServer)
* [DBConnector](./4Cource/DBConnector)
* [TLS\_MemoryPool](./4Cource/TLS_MemoryPool)
* [TLS\_SerializeBufferPtr](./4Cource/TLS_SerializeBufferPtr)

\---

### 🔧 Utils

* [MemoryPool](./Utils/MemoryPool)
* [RingBuffer](./Utils/RingBuffer)
* [SerializeBuffer](./Utils/SerializeBuffer)
* [MultiThreadProfiler](./3Cource/Profiler_TLS/NoReset/CustomProfiler_TLS)
* [CacheSimulator](./Utils/CacheSimulator)
* [CrashDump](./Utils/CrashDump)

\---

## 4Cource 중심 설명

### GameServer

* Field 기반 구조로 구성된 게임 서버
* Auth / Echo Field로 역할 분리
* GameManager를 통한 Field 관리

👉 [바로가기](./4Cource/GameServer)

\---

### LoginServer

* 로그인 처리 + DB 연동
* 인증 흐름 및 DB 작업 분리

👉 [바로가기](./4Cource/LoginServer/LoginServer)

\---

### ChattingServer

* 버전별로 발전 과정 정리
* 멀티스레드 구조 변화 확인 가능

👉 [바로가기](./4Cource/ChattingServer)

\---

### MonitoringServer

* 서버 상태 모니터링 분리
* 로컬 모니터링 + 서버 모니터링 구조

👉 [바로가기](./4Cource/MonitoringServer/MonitoringServer)

