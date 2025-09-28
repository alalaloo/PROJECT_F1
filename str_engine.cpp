#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <atomic>
#include <ncurses.h>

struct engine {
private:
    const double max_rpm = 15000.0;
    const double deceleration_rate = 500.0;
    const double acceleration_rate_max = 3000.0;
    const double time_to_max_rpm = 5.0;
    const double max_torque = 500.0;
    const double peak_rpm = 11000.0;
    const double null_rpm = 4000.0;

    double rpm = 0;
    double torque = 0;

public:
    void calculate_rpm_torque(bool pedal_pos) {
        // Дифференциальное время
        const double dt = 0.01;

        double sigma_factor;
        if ((rpm > 0) && (rpm < max_rpm/3) || (rpm > max_rpm/3*2) && (rpm < max_rpm)) {
            sigma_factor = 0.5 * acceleration_rate_max;
        }
        else {
            sigma_factor = acceleration_rate_max;
        }
        
        if (pedal_pos) {
            rpm = rpm + dt * sigma_factor;
            if (rpm > max_rpm) {
                rpm = max_rpm;
            }
        } else {
            rpm = rpm - dt * deceleration_rate;
            if (rpm < 0) {
                rpm = 0;
            }
        }
        
        if (rpm < null_rpm) {
            torque = 0;
        }
        else if (rpm <= peak_rpm) {
            torque = max_torque * (rpm / peak_rpm);
        }
        else {
            double drop_factor = 1.0 - 0.4 * (rpm - peak_rpm) / (max_rpm - peak_rpm);
            torque = max_torque * drop_factor;
        }
    }
    
    double get_rpm() const { return rpm; }
    double get_torque() const { return torque; }
    double get_max_rpm() const { return max_rpm; }
    void set_rpm(double new_rpm) { rpm = new_rpm; }
};

int main() {
    // Инициализация ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    
    engine f1_engine;
    
    std::atomic<bool> running(true);
    std::atomic<bool> gas_pressed(false);
    
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point last_press_time = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point last_release_time = std::chrono::steady_clock::now();
    
    // Поток для обновления оборотов
    std::thread rpm_thread([&]() {
        while (running) {
            // Используем метод calculate_rpm_torque для обновления RPM
            f1_engine.calculate_rpm_torque(gas_pressed);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // dt = 0.01 секунды
        }
    });
    
    // Основной цикл обработки ввода
    while (running) {
        clear();
        
        // Выводим информацию
        mvprintw(0, 0, "Formula 1 Engine RPM Simulator");
        mvprintw(1, 0, "==============================");
        mvprintw(2, 0, "Current RPM: %.0f", f1_engine.get_rpm());
        mvprintw(3, 0, "Current Torque: %.1f Nm", f1_engine.get_torque());
        mvprintw(4, 0, "Gas pedal: %s", gas_pressed ? "PRESSED (W)" : "RELEASED");
        mvprintw(5, 0, "Progress: %.1f%%", (f1_engine.get_rpm() / f1_engine.get_max_rpm()) * 100);
        mvprintw(8, 0, "Controls:");
        mvprintw(9, 0, "W - Hold for gas");
        mvprintw(10, 0, "R - Reset RPM");
        mvprintw(11, 0, "ESC - Exit");
        
        // Обработка ввода
        int ch = getch();
        
        switch (ch) {
            case 'w': // W - педаль газа
            case 'W':
                if (!gas_pressed) {
                    gas_pressed = true;
                    last_press_time = std::chrono::steady_clock::now();
                }
                break;
            case 'r': // R - сбросить RPM
            case 'R':
                f1_engine.set_rpm(0.0);
                gas_pressed = false;
                last_press_time = std::chrono::steady_clock::now();
                last_release_time = std::chrono::steady_clock::now();
                break;
            case 27: // ESC - выход
                running = false;
                break;
        }
        
        // Если W отпущена
        if (ch != 'w' && ch != 'W' && gas_pressed) {
            gas_pressed = false;
            last_release_time = std::chrono::steady_clock::now();
        }
        
        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    
    // Очистка
    running = false;
    if (rpm_thread.joinable()) {
        rpm_thread.join();
    }
    
    endwin();
    std::cout << "Engine simulation stopped." << std::endl;
    
    return 0;
}
