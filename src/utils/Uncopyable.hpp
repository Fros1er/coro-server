#pragma once

class Uncopyable {
public:
    Uncopyable() = default;
    Uncopyable(Uncopyable &) = delete;
    Uncopyable &operator=(Uncopyable &) = delete;
};