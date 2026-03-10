package com.raylib.raymob;

public final class NativeLoader {
    static {
        System.loadLibrary("raymob");
    }

    private NativeLoader() {
    }
}
