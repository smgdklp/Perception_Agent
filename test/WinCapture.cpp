class WinCapture {
public :
	WinCapture();
	~WinCapture();

	int Command(int command, void* input, void* output) {
		switch (command) {
		case 1:
			SetHWND();
			return 0;
		case 2:
			// Handle command 2
			return 0;
		default:
			// Handle unknown command
			return -1;
		};
	private:
		void SetHWND() {
			
		}
		
		HWND hwnd;
		// 其它成员...
	};


// 用 static
static void helper() { /*...*/ }

// 或用匿名命名空间
namespace {
    void helper() { /*...*/ }
}