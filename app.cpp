#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <set>
#include <cmath>
#include <cstdlib>

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

#define NOMINMAX

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

ImVec2 operator+(ImVec2& lhs, ImVec2& rhs) {
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}
ImVec4 operator*(const ImVec4& lhs, float rhs) {
    return ImVec4(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
}

void InputTextWithHint(const std::string& label, const std::string& hint, std::string& data, ImGuiInputTextFlags flags = 0) {
    char buffer[256];
    strncpy(buffer, data.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    ImGui::InputTextWithHint(label.c_str(), hint.c_str(), buffer, sizeof(buffer), flags);
    data = buffer;
}

float GetDPI(GLFWwindow* window){
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();

    int width_mm, height_mm;
    glfwGetMonitorPhysicalSize(monitor, &width_mm, &height_mm);

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int screenWidth = mode->width;
    int screenHeight = mode->height;

    float dpiX = (float)screenWidth / width_mm;
    float dpiY = (float)screenHeight / height_mm;
    return std::max(dpiX, dpiY);
}


std::set<char> ops = { '+', '-', 'X', '/' };
float dpi = 100;
bool egypt = false;

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

const std::string EGYPT_ALPH = "ABCDEF"; // max 1e5 !!!
std::string decToEgypt(int val) {
    std::string numb = std::to_string(val);
    reverse(numb.begin(), numb.end());
    std::string res = "";
    for (int i = 0; i < numb.size(); ++i) {
        for (int j = 0; j < numb[i] - '0'; ++j) {
            res += EGYPT_ALPH[i];
        }
    }
    reverse(res.begin(), res.end());
    return res;
}

int egyptToDec(std::string numb) {
    int res = 0;
    for (auto& el : numb) {
        res += pow(10, el - 'A');
    }
    return res;
}


void eval(std::string& output, const int& base, const int& accuracy) {
    bool commaAlready = false;
    bool first = true;

    long double dAns = 0.f;
    long double dRhs = 0.f;
    long double order = 1.f;

	std::string egyptStrL = "";
	std::string egyptStrR = "";

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
            if (egypt) {
                egyptStrL += el;
            }
            else {
                dAns *= base;
                dAns += ('0' <= el && el <= '9') ? (el - '0') : (el - 'A' + 10);
            }
        }
        else {
			if (egypt) {
				egyptStrR += el;
			}
			else {
                dRhs *= base;
                dRhs += ('0' <= el && el <= '9') ? (el - '0') : (el - 'A' + 10);
			}
        }
    }
    if (egypt) {
        if (op == '*') {
			output = decToEgypt(egyptToDec(egyptStrL) * egyptToDec(egyptStrR));
        }
        if (op == '+') {
			output = decToEgypt(egyptToDec(egyptStrL) + egyptToDec(egyptStrR));
        }
        if (op == '-') {
			output = decToEgypt(egyptToDec(egyptStrL) - egyptToDec(egyptStrR));
        }
    }
    else {
        dAns += std::numeric_limits<float>::epsilon();
        dRhs *= order;
        if (op == '*') {
            dAns *= dRhs;
        }
        if (op == '/') {
            if (dRhs == 0) { output = ""; return; }
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
}

void digitButton(std::string& output, char d, ImVec2 wSize, int base, ImGuiStyle& style) {
    ImVec4 color1 = style.Colors[ImGuiCol_Button];
    ImVec4 color2 = style.Colors[ImGuiCol_Text];
    if (!egypt && digitToInt(d) >= base ||
        egypt && (0 <= digitToInt(d) && digitToInt(d) <= 9)) {
        float w1 = style.Colors[ImGuiCol_Button].w;
        float w2 = style.Colors[ImGuiCol_Text].w;
        style.Colors[ImGuiCol_Button] = style.Colors[ImGuiCol_Button] * 0.7f;
        style.Colors[ImGuiCol_Text] = style.Colors[ImGuiCol_Text] * 0.6f;

        style.Colors[ImGuiCol_Button].w = w1;
        style.Colors[ImGuiCol_Text].w = w2;
    }
    if (ImGui::Button(std::string(1, d).c_str(), ImVec2(dpi / 2, dpi / 2)) &&
        !(!egypt && (digitToInt(d) >= base ||
          ((output == "0" || output.back() == '0' && ops.find(output[output.size() - 2]) != ops.end()) && d == '0')) ||
          egypt && (0 <= digitToInt(d) && digitToInt(d) <= 9))) {
        if (output == "0") {
            output = "";
        }
        output += d;
    }
    style.Colors[ImGuiCol_Button] = color1;
    style.Colors[ImGuiCol_Text] = color2;
}

void operatorButton(std::string& output, char oper, bool& op, bool& commaAlready, int base, int accuracy, ImVec2 wSize, ImGuiStyle& style) {
    ImVec4 color1 = style.Colors[ImGuiCol_Button];
    ImVec4 color2 = style.Colors[ImGuiCol_Text];
    if (egypt && oper == '/') {
        float w1 = style.Colors[ImGuiCol_Button].w;
        float w2 = style.Colors[ImGuiCol_Text].w;
        style.Colors[ImGuiCol_Button] = style.Colors[ImGuiCol_Button] * 0.7f;
        style.Colors[ImGuiCol_Text] = style.Colors[ImGuiCol_Text] * 0.6f;

        style.Colors[ImGuiCol_Button].w = w1;
        style.Colors[ImGuiCol_Text].w = w2;
    }
    if (ImGui::Button(std::string(1, oper).c_str(), ImVec2(dpi / 2, dpi / 2)) && !(egypt && oper == '/')) {
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
    style.Colors[ImGuiCol_Button] = color1;
    style.Colors[ImGuiCol_Text] = color2;
}


int main(int argc, char* argv[])
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
    GLFWwindow* window = glfwCreateWindow(2.61f * dpi, 2.82f * dpi, "Calculator", nullptr, nullptr);

    dpi = 20 * GetDPI(window);
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
    ImGuiStyle& style = ImGui::GetStyle();

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

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize({ 3.06f * dpi, 3.3f * dpi });
		bool open = true;
        ImGui::Begin("Calculator", &open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
        ImGui::SetWindowFontScale(dpi / 50.);

        ImGui::SetNextItemWidth(2.87f * dpi);
        InputTextWithHint("##Calculations", "", output, ImGuiInputTextFlags_ReadOnly);

        if (ImGui::Button("Cl", ImVec2(dpi / 2, dpi / 2))) {
            output = "0";
			commaAlready = false;
			op = false;
        } ImGui::SameLine();
        if (ImGui::Button("NS", ImVec2(dpi / 2, dpi / 2))) {
            baseTmp = base;
            openNumSysWindow = true;
        } ImGui::SameLine();
        if (ImGui::Button("Ac", ImVec2(dpi / 2, dpi / 2))) {
            accuracyTmp = accuracy;
            openAccuracyWindow = true;
		} ImGui::SameLine();
        ImVec4 color1 = style.Colors[ImGuiCol_Button];
        ImVec4 color2 = style.Colors[ImGuiCol_Text];
        if (egypt) {
            float w1 = style.Colors[ImGuiCol_Button].w;
            float w2 = style.Colors[ImGuiCol_Text].w;
            style.Colors[ImGuiCol_Button] = style.Colors[ImGuiCol_Button] * 0.7f;
            style.Colors[ImGuiCol_Text] = style.Colors[ImGuiCol_Text] * 0.6f;

            style.Colors[ImGuiCol_Button].w = w1;
            style.Colors[ImGuiCol_Text].w = w2;
        }
        if (ImGui::Button(".", ImVec2(dpi / 2, dpi / 2)) && !commaAlready) {
            output += '.';
            commaAlready = true;
        } ImGui::SameLine();
        style.Colors[ImGuiCol_Button] = color1;
        style.Colors[ImGuiCol_Text] = color2;
        if (ImGui::Button("=", ImVec2(dpi / 2, dpi / 2)) && ops.find(output.back()) == ops.end() && op) {
            eval(output, base, accuracy);
            op = false;
            commaAlready = false;
        }
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + dpi / 50);

        digitButton(output, 'C', wSize, base, style); ImGui::SameLine();
		digitButton(output, 'D', wSize, base, style); ImGui::SameLine();
        digitButton(output, 'E', wSize, base, style); ImGui::SameLine();
        digitButton(output, 'F', wSize, base, style); ImGui::SameLine();
        operatorButton(output, '+', op, commaAlready, base, accuracy, wSize, style);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + dpi / 50);

        digitButton(output, '8', wSize, base, style); ImGui::SameLine();
        digitButton(output, '9', wSize, base, style); ImGui::SameLine();
        digitButton(output, 'A', wSize, base, style); ImGui::SameLine();
        digitButton(output, 'B', wSize, base, style); ImGui::SameLine();
        operatorButton(output, '-', op, commaAlready, base, accuracy, wSize, style);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + dpi / 50);

		digitButton(output, '4', wSize, base, style); ImGui::SameLine();
		digitButton(output, '5', wSize, base, style); ImGui::SameLine();
		digitButton(output, '6', wSize, base, style); ImGui::SameLine();
		digitButton(output, '7', wSize, base, style); ImGui::SameLine();
        operatorButton(output, '*', op, commaAlready, base, accuracy, wSize, style);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + dpi / 50);

		digitButton(output, '0', wSize, base, style); ImGui::SameLine();
		digitButton(output, '1', wSize, base, style); ImGui::SameLine();
		digitButton(output, '2', wSize, base, style); ImGui::SameLine();
		digitButton(output, '3', wSize, base, style); ImGui::SameLine();
        operatorButton(output, '/', op, commaAlready, base, accuracy, wSize, style);


        if (openNumSysWindow) {
            ImGui::Begin("Numerical system");
            ImGui::SetWindowFontScale(dpi / 50.);

            ImGui::Text("Choose the base (from 2 to 16 only)");
            ImGui::InputInt("Base", &baseTmp, 1, 4); ImGui::SameLine();
            ImGui::Checkbox("Egyptian NS", &egypt);
            if (ImGui::Button("Cancel")) {
                openNumSysWindow = false;
                egypt = false;
            }
            if (ImGui::Button("Confirm") && baseTmp >= 2 && baseTmp <= 16) {
                openNumSysWindow = false;
                base = baseTmp;
                op = false;
                commaAlready = false;
                output = "0";
            }

            ImGui::End();
        }

        if (openAccuracyWindow) {
            ImGui::Begin("Accuracy");
            ImGui::SetWindowFontScale(dpi / 50.);

            ImGui::Text("Number of decimal places (from 0 to 5 only)");
            ImGui::InputInt("Accuracy", &accuracyTmp, 1, 1);
            if (ImGui::Button("Cancel")) {
                openAccuracyWindow = false;
            }
            if (ImGui::Button("Confirm") && accuracyTmp >= 0 && accuracyTmp <= 5) {
                openAccuracyWindow = false;
                accuracy = accuracyTmp;
                op = false;
                commaAlready = false;
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
