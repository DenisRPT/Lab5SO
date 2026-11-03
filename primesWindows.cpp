#include <windows.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static const int PROCESE = 10;
static const int PAS = 1000;

static int worker() {
    long long lo, hi;
    if (!(std::cin >> lo >> hi)) return 1;

    std::vector<bool> prim(hi - lo + 1, true);
    for (long long n = lo; n < 2 && n <= hi; ++n) prim[n - lo] = false;

    for (long long d = 2; d * d <= hi; ++d) {
        long long start = std::max(d * d, (lo + d - 1) / d * d);
        for (long long m = start; m <= hi; m += d) prim[m - lo] = false;
    }
    for (long long n = lo; n <= hi; ++n)
        if (prim[n - lo]) std::cout << n << '\n';
    return 0;
}

struct Child {
    HANDLE proces;
    HANDLE scriere;
    HANDLE citire;
};

static Child start_worker() {
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};

    HANDLE inR, inW, outR, outW;
    CreatePipe(&inR, &inW, &sa, 0);
    CreatePipe(&outR, &outW, &sa, 0);

    SetHandleInformation(inW, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(outR, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = inR;
    si.hStdOutput = outW;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    char exe[MAX_PATH];
    GetModuleFileNameA(nullptr, exe, MAX_PATH);
    std::string comanda = std::string("\"") + exe + "\" --worker";

    PROCESS_INFORMATION pi{};
    if (!CreateProcessA(nullptr, &comanda[0], nullptr, nullptr,
                        TRUE, 0, nullptr, nullptr, &si, &pi)) {
        std::cerr << "CreateProcess failed, code " << GetLastError() << "\n";
        ExitProcess(1);
    }

    CloseHandle(pi.hThread);
    CloseHandle(inR);
    CloseHandle(outW);

    return Child{pi.hProcess, inW, outR};
}

static void send_intervals(Child &c, int lo, int hi) {
    std::string text = std::to_string(lo) + " " + std::to_string(hi) + "\n";
    DWORD scris;
    WriteFile(c.scriere, text.data(), (DWORD)text.size(), &scris, nullptr);
    CloseHandle(c.scriere);
    c.scriere = nullptr;
}

static std::string read_all(Child &c) {
    std::string rezultat;
    char buf[512];
    DWORD citit;
    while (ReadFile(c.citire, buf, sizeof(buf), &citit, nullptr) && citit > 0)
        rezultat.append(buf, citit);
    CloseHandle(c.citire);
    return rezultat;
}

int main(int argc, char **argv) {
    if (argc > 1 && std::string(argv[1]) == "--worker") return worker();

    std::vector<Child> copii;
    for (int i = 0; i < PROCESE; ++i) {
        Child c = start_worker();
        send_intervals(c, i * PAS, i * PAS + PAS - 1);
        copii.push_back(c);
    }

    for (int i = 0; i < PROCESE; ++i) {
        std::string raspuns = read_all(copii[i]);
        WaitForSingleObject(copii[i].proces, INFINITE);
        CloseHandle(copii[i].proces);

        std::cout << "--- proces " << i << " [" << i * PAS << ", " << i * PAS + PAS - 1 << "] ---\n";
        std::istringstream in(raspuns);
        int p;
        while (in >> p) std::cout << p << ' ';
        std::cout << "\n";
    }
    return 0;
}