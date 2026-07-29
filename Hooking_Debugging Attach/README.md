# Debugger-Based API 후킹

* 작성자 : 2N(nms200299)
* 블로그 포스팅 (개념 정리) :

  * https://blog.naver.com/nms200299/224362158122

### 시연 영상 :

https://github.com/user-attachments/assets/f2ecd7ed-8a80-47ac-9d98-3cce3f1b3337

### 구현 내용 :

* Static/Dynamic Link에 따른 함수 브레이크 포인트(INT3) 삽입
* MessageBoxW/A 함수를 후킹하여 Parm2, Parm3 변조
* EXCEPTION_SINGLE_STEP 처리를 통한 재후킹

### 테스트 결과 :

| OS 종류 | OS 아키텍처 | PE 아키텍처 | 디버깅 후킹 결과 (Static Link) | 디버깅 후킹 결과 (Dynamic Link) |
| :--- | :--- | :--- | :---: | :---: |
| **Windows 7** | x86 | x86 | O | O |
| | x64 | x86 (WoW64) | O | O |
| | x64 | x64 | O | O |
| **Windows 8 (Update 3)** | x86 | x86 | O | O |
| | x64 | x86 (WoW64) | O | O |
| | x64 | x64 | O | O |
| **Windows 10** | x86 | x86 | O | O |
| | x64 | x86 (WoW64) | O | O |
| | x64 | x64 | O | O |
| **Windows 11** | x64 | x86 (WoW64) | O | O |
| | x64 | x64 | O | O |

#### 표기 기준

* O : 정상 동작
* △ : 보안 기법(CFG, CET 등)으로 인해 제한적 또는 우회 필요
* \- : 구조적으로 미지원
