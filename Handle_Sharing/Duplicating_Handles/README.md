# 복제를 통한 커널 오브젝트 핸들 공유

* 작성자 : 2N(nms200299)
* 블로그 포스팅 (개념 정리) :

  * https://blog.naver.com/nms200299/224372366042

### 테스트 :

* Windows 11 25H2 x64

<img width="450" height="200" alt="IMG" src="https://github.com/user-attachments/assets/b5f1369f-a49c-46e0-b8ab-334e44c48a85" />

### 구현 내용 :

|프로그램 이름|구현 내용|
|-|-|
|SrcProgram|- 파일 핸들 발급 및 복제|
||- Named Pipe IPC를 이용해 복제된 파일 핸들 값 전송|
|DstProgram|- Named Pipe IPC 생성 및 연결 대기|
||- IPC를 통해 핸들 값을 수신하여 WriteFile() 호출|
