#include <print>
#include <exception>

import Nandina.Window;
import Nandian.Application;

using namespace Nandina;


int main() {
    try {
        NanApplication app;

        const NanWindow window = NanWindow::Builder()
            .set_title("Hello, Nandina Framework!")
            .set_width(1024)
            .set_height(768)
            .build();

        // 3. 运行窗口
        window.run();

    } catch (const std::exception& e) {
        std::println(stderr, "Fatal error: {}", e.what());
        return 1;
    }

    return 0;
}