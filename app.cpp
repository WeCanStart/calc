#define NOMINMAX
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <set>
#include <cmath>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

ImVec2 operator+(ImVec2& lhs, ImVec2& rhs) {
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

void InputTextWithHint(const std::string& label, const std::string& hint, std::string& data, ImGuiInputTextFlags flags = 0) {
    char buffer[256];
    strncpy(buffer, data.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    ImGui::InputTextWithHint(label.c_str(), hint.c_str(), buffer, sizeof(buffer), flags);
    data = buffer;
}

std::set<char> ops = { '+', '-', 'X', '/' };

char intToDigit(const int& i) {
    if (0 <= i && i <= 9) {
        return i + '0';
    }
    else {
        return 'A' + i - 10;
    }
}
int digitToInt(const char& c) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    }
    else {
        return c - 'A' + 10;
    }
}

void eval(std::string& output, const int& base, const int& accuracy) {
    bool commaAlready = false;
    bool first = true;

    long double dAns = 0.f;
    long double dRhs = 0.f;
    long double order = 1.f;

    char op = 0;

    for (const auto& el : output) {
        if (el == '*' || el == '/' || el == '+' || el == '-') {
            op = el;
            dAns *= order;

            first = false;
            commaAlready = false;
            order = 1.f;
            continue;
        }
        if (el == '.') {
            commaAlready = true;
            continue;
        }
        if (commaAlready) {
            order /= base;
        }
        if (first) {
            dAns *= base;
            dAns += ('0' <= el && el <= '9') ? (el - '0') : (el - 'A' + 10);
        }
        else {
            dRhs *= base;
            dRhs += ('0' <= el && el <= '9') ? (el - '0') : (el - 'A' + 10);
        }
    }
	dAns += 0.00001f;
    dRhs *= order;
    if (op == '*') {
        dAns *= dRhs;
    }
    if (op == '/') {
        dAns /= dRhs;
    }
    if (op == '+') {
        dAns += dRhs;
    }
    if (op == '-') {
        dAns -= dRhs;
        dAns = std::abs(dAns);
    }
    long double maxPower = 1.f / std::powl(base, accuracy);
	if (dAns > maxPower) { //looks crazy, but it works
        while (dAns > maxPower) {
            maxPower *= base;
        }
        maxPower /= base;
    }
    output = "";
    commaAlready = false;
    if (maxPower < 0.9) {
        output += '0';
        if (accuracy) output += '.';
        commaAlready = true;
    }
    long double dAnsCp = dAns;
	maxPower = std::max(maxPower, 1. / (long double)base);
    while (maxPower * 1.5 > 1.f / std::powl(base, accuracy)) {
        int digit = 0;
        while (dAnsCp > maxPower) {
            dAnsCp -= maxPower;
            ++digit;
        }
        maxPower /= base;
        output += intToDigit(digit);
        if (maxPower < 0.99 && !commaAlready && accuracy) {
            output += ".";
            commaAlready = true;
        }
    }
}

void digitButton(std::string& output, char d, ImVec2 wSize) {
    if (ImGui::Button(std::string(1, d).c_str(), ImVec2(40 * wSize.x / 1920, 40 * wSize.y / 1440))) {
        if ((output == "0" || output.back() == '0' && ops.find(output[output.size() - 2]) != ops.end()) && d == '0') {
            return;
        }
        if (output == "0") {
            output = "";
        }
        output += d;
    }
}

void operatorButton(std::string& output, char oper, bool& op, bool& commaAlready, int base, int accuracy, ImVec2 wSize) {
    if (ImGui::Button(std::string(1, oper).c_str(), ImVec2(40 * wSize.x / 1920, 40 * wSize.y / 1440))) {
        if (ops.find(output.back()) != ops.end()) {
            output.back() = oper;
        }
        else {
            if (op) {
                eval(output, base, accuracy);
            }
            commaAlready = false;
            op = true;
            output += oper;
        }
    }
}


int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    GLFWwindow* window = glfwCreateWindow(3000, 2000, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    std::string output = "0";
    int base = 10;
    int accuracy = 0;

    int accuracyTmp = -1;
    int baseTmp = -1;

    bool openNumSysWindow = false;
    bool openAccuracyWindow = false;

    bool op = false;
    bool commaAlready = false;


#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImVec2 dSize = io.DisplaySize;
        int tmp1, tmp2;
        glfwGetWindowSize(window, &tmp1, &tmp2);
        ImVec2 wSize(tmp1, tmp2);

        ImGui::SetNextWindowSize({ 0.15f * wSize.x, 0.25f * wSize.y });
        ImGui::Begin("Calculator");
        ImGui::SetWindowFontScale(1.2);

        InputTextWithHint("Calculations", "", output, ImGuiInputTextFlags_ReadOnly);

        if (ImGui::Button("Cl", ImVec2(40 * wSize.x / 1920, 40 * wSize.y / 1440))) {
            output = "0";
			commaAlready = false;
			op = false;
        } ImGui::SameLine();
        if (ImGui::Button("NS", ImVec2(40 * wSize.x / 1920, 40 * wSize.y / 1440))) {
            baseTmp = base;
            openNumSysWindow = true;
        } ImGui::SameLine();
        if (ImGui::Button("Ac", ImVec2(40 * wSize.x / 1920, 40 * wSize.y / 1440))) {
            accuracyTmp = accuracy;
            openAccuracyWindow = true;
		} ImGui::SameLine();
        if (ImGui::Button(".", ImVec2(40 * wSize.x / 1920, 40 * wSize.y / 1440)) && !commaAlready) {
            output += '.';
            commaAlready = true;
        } ImGui::SameLine();
        if (ImGui::Button("=", ImVec2(40 * wSize.x / 1920, 40 * wSize.y / 1440)) && ops.find(output.back()) == ops.end() && op) {
            eval(output, base, accuracy);
            op = false;
            commaAlready = false;
        }

        if (digitToInt('C') < base) { digitButton(output, 'C', wSize);  }
		if (digitToInt('D') < base) { ImGui::SameLine(); digitButton(output, 'D', wSize); }
        if (digitToInt('E') < base) { ImGui::SameLine(); digitButton(output, 'E', wSize); }
        if (digitToInt('F') < base) { ImGui::SameLine(); digitButton(output, 'F', wSize); }

        if (digitToInt('8') < base) { digitButton(output, '8', wSize); }
        if (digitToInt('9') < base) { ImGui::SameLine(); digitButton(output, '9', wSize); }
        if (digitToInt('A') < base) { ImGui::SameLine(); digitButton(output, 'A', wSize); }
        if (digitToInt('B') < base) { ImGui::SameLine(); digitButton(output, 'B', wSize); }

        if (digitToInt('4') < base) { digitButton(output, '4', wSize); }
        if (digitToInt('5') < base) { ImGui::SameLine(); digitButton(output, '5', wSize); }
        if (digitToInt('6') < base) { ImGui::SameLine(); digitButton(output, '6', wSize); }
        if (digitToInt('7') < base) { ImGui::SameLine(); digitButton(output, '7', wSize); }

        if (digitToInt('0') < base) { digitButton(output, '0', wSize); }
        if (digitToInt('1') < base) { ImGui::SameLine(); digitButton(output, '1', wSize); }
        if (digitToInt('2') < base) { ImGui::SameLine(); digitButton(output, '2', wSize); }
        if (digitToInt('3') < base) { ImGui::SameLine(); digitButton(output, '3', wSize); }
        
		operatorButton(output, '+', op, commaAlready, base, accuracy, wSize); ImGui::SameLine();
        operatorButton(output, '-', op, commaAlready, base, accuracy, wSize); ImGui::SameLine();
        operatorButton(output, '*', op, commaAlready, base, accuracy, wSize); ImGui::SameLine();
        operatorButton(output, '/', op, commaAlready, base, accuracy, wSize);


        if (openNumSysWindow) {
            ImGui::Begin("Numerical system");

            ImGui::Text("Choose the base (from 2 to 16 only)");
            ImGui::InputInt("Base", &baseTmp, 1, 4);
            if (ImGui::Button("Cancel")) {
                openNumSysWindow = false;
            }
            if (ImGui::Button("Confirm") && baseTmp >= 2 && baseTmp <= 16) {
                openNumSysWindow = false;
                base = baseTmp;
                output = "0";
            }

            ImGui::End();
        }

        if (openAccuracyWindow) {
            ImGui::Begin("Accuracy");

            ImGui::Text("Number of decimal places (from 0 to 5 only)");
            ImGui::InputInt("Accuracy", &accuracyTmp, 1, 1);
            if (ImGui::Button("Cancel")) {
                openAccuracyWindow = false;
            }
            if (ImGui::Button("Confirm") && accuracyTmp >= 0 && accuracyTmp <= 5) {
                openAccuracyWindow = false;
                accuracy = accuracyTmp;
                output = "0";
            }

            ImGui::End();
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
