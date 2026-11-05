
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

static const int PROCESE = 10;
static const int PAS = 1000;

struct Interval {
    int lo, hi;
};

struct Copil {
    pid_t pid;
    int read;
};

static bool isPrim(int n) {
    if (n < 2) return false;
    for (int d = 2; d * d <= n; ++d)
        if (n % d == 0) return false;
    return true;
}

static bool readEx(int fd, void *dest, size_t n) {
    char *p = static_cast<char *>(dest);
    while (n > 0) {
        ssize_t r = read(fd, p, n);
        if (r <= 0) return false;
        p += r;
        n -= r;
    }
    return true;
}

[[noreturn]] static void copil(int fd_interval, int fd_rezultat) {
    Interval iv;
    if (!readEx(fd_interval, &iv, sizeof(iv))) _exit(1);
    close(fd_interval);

    std::vector<int> primes;
    for (int n = iv.lo; n <= iv.hi; ++n)
        if (isPrim(n)) primes.push_back(n);

    write(fd_rezultat, primes.data(), primes.size() * sizeof(int));
    close(fd_rezultat);
    _exit(0);
}

int main() {
    std::vector<Copil> copii;

    for (int i = 0; i < PROCESE; ++i) {
        int to[2], from[2];
        if (pipe(to) == -1 || pipe(from) == -1) {
            perror("pipe");
            return 1;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            close(to[1]); 
            close(from[0]);
            for (const Copil &c : copii) close(c.read);
            copil(to[0], from[1]);
        }

        close(to[0]);
        close(from[1]);

        Interval iv{i * PAS, i * PAS + PAS - 1};
        write(to[1], &iv, sizeof(iv));
        close(to[1]);

        copii.push_back(Copil{pid, from[0]});
    }

    for (int i = 0; i < PROCESE; ++i) {
        std::cout << "--- proces " << i << " [" << i * PAS << ", " << i * PAS + PAS - 1 << "] ---\n";

        int p;
        while (readEx(copii[i].read, &p, sizeof(p))) std::cout << p << ' ';
        std::cout << "\n";

        close(copii[i].read);
        waitpid(copii[i].pid, nullptr, 0);
    }
    return 0;
}