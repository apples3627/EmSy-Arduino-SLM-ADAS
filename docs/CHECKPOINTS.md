# 프로젝트 Git 체크포인트

## 2026-10-10: ADAS 역할 변경 전

기존 구조는 MAIN + COMPUTE_I2C H32 + COMPUTE_SPI H32 + ACTUATOR다. 모델은 V4 `95616d56`, 최대4개의 명시적 영어 명령을 계획한다. 독립 모델/호스트 통합 검사와 Uno 컴파일 기록은 intent/MULTI_VERIFICATION.md 및 firmware/reports/VERIFICATION.md에 있다. V4의 실제 네 보드 실물 검증은 아직 수행하지 않았다.

이 변경 전 상태를 `pre-adas-2026-10-10` 태그로 보관하고 GitHub 비공개 저장소에 먼저 업로드한다. 이후 검증 완료한 새 구조는 MAIN + SLM_FULL_SPI + ADAS_I2C + ACTUATOR로 별도 커밋/태그를 남긴다.

macOS .DS_Store와 사용자가 풀어둔 firmware/packages/compute_i2c/의 미추적 사본은 프로젝트 정본에 추가하지 않고 로컬에 그대로 보존한다. 기록된 소스·모델·데이터·문서·배포 ZIP만 업로드한다.
