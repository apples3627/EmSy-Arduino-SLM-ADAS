# 프로젝트 Git 체크포인트

## 2026-10-10: ADAS 역할 변경 전

기존 구조는 MAIN + COMPUTE_I2C H32 + COMPUTE_SPI H32 + ACTUATOR다. 모델은 V4 `95616d56`, 최대4개의 명시적 영어 명령을 계획한다. 독립 모델/호스트 통합 검사와 Uno 컴파일 기록은 intent/MULTI_VERIFICATION.md 및 firmware/reports/VERIFICATION.md에 있다. V4의 실제 네 보드 실물 검증은 아직 수행하지 않았다.

이 변경 전 상태를 `pre-adas-2026-10-10` 태그로 보관하고 GitHub 비공개 저장소에 먼저 업로드한다. 이후 검증 완료한 새 구조는 MAIN + SLM_FULL_SPI + ADAS_I2C + ACTUATOR로 별도 커밋/태그를 남긴다.

macOS .DS_Store와 사용자가 풀어둔 firmware/packages/compute_i2c/의 미추적 사본은 프로젝트 정본에 추가하지 않고 로컬에 그대로 보존한다. 기록된 소스·모델·데이터·문서·배포 ZIP만 업로드한다.

GitHub 비공개 저장소: https://github.com/apples3627/EmSy-Arduino-SLM-ADAS (생성 후 isPrivate=true/visibility=PRIVATE 확인). 변경 전 커밋26e2ab0와 태그의 원격 업로드를 완료했다.

## 2026-10-10: ADAS 검증 완료 후

`adas-verified-2026-10-10` 태그는 MAIN + full SLM_SPI + HC-SR04 ADAS_I2C + ACTUATOR_UART 구조의 검증 완료 배포를 가리킵니다.20cm 정지,25cm 이상3회 해제, 공통 ALERT 인터럽트, 안전 epoch 및 desired/applied snapshot을 포함합니다. 동일 모델95616d56을 사용하며 재학습은 없습니다.

독립 검증8개, 구현 레지스터 회귀2개, Python 모델33개 parity, 네 Uno 빌드와 배포 ZIP4개 hash/독립컴파일을 통과했습니다. 실제 네 보드 업로드·파형·스택 최고값은 미검증입니다. 변경 전 태그와 최종 태그 모두 비공개 origin에 보관합니다.
