#include "test.C"

void run_tests(void) {
	S_PRINT("Running tests\n");
	S_RUN_TEST(OpenValidFile);
	S_RUN_TEST(OpenInvalidFile);
	S_RUN_TEST(OpenDirectory);
	S_RUN_TEST(OpenRTC);
	S_RUN_TEST(CloseOpenDescriptor);
	S_RUN_TEST(CloseTerminal);
	S_RUN_TEST(CloseInvalidDescriptor);
	S_RUN_TEST(MaxOutDescriptors);
	S_RUN_TEST(OpenRTCTwice);
	S_RUN_TEST(ReadFromInvalidDescriptors);
	S_RUN_TEST(ReadFromDirectoryNormal);
	S_RUN_TEST(ReadFromDirectoryShortBuffer);
	S_RUN_TEST(ReadFromDirectoryWeirdParameters);
	S_RUN_TEST(ReadAllFiles);
	S_RUN_TEST(ReadFileInChunks);
	S_RUN_TEST(ReadFileWeirdParameters);
	S_RUN_TEST(ReadFilePastEOF);
	S_RUN_TEST(WriteToTerminal);
	S_RUN_TEST(WriteToFilesystem);
	S_RUN_TEST(TestReadRTC);
	S_RUN_TEST(TestInvalidWriteRTC);
	S_RUN_TEST(TestValidWriteRTC);
	S_RUN_TEST(TestOpenRTCSetsDefaultRate);
	S_PRINT("All tests successful\n");
}
