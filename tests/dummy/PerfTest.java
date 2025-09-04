package dummy;

import java.time.Instant;
import java.time.Duration;

public class PerfTest {
    static void doSomething(int i) {
        int calc = i * 2;
        System.out.print(" <CALCULATION: " + calc + ">");
    }

    static void stress() {
        var start = Instant.now();
        System.out.println();
        for (int i = 1; i <= 1_000_000; ++i) {
            System.out.print("\rIteration: " + i);
            System.out.flush();
            doSomething(i);
            System.out.flush();
        }
        System.out.println();
        var stop = Instant.now();

        Duration elapsed = Duration.between(start, stop);

        System.out.println("Time to run: " + elapsed.toMillis() + "ms");
        System.out.println();
    }
    
    public static void main(String[] args) throws Exception {
        System.out.println("[*] Started Performance Test");

        if (args.length == 0) {
            System.out.println("Missing library path!");
            return;
        }

        System.out.println("[CLEAN]");
        stress();

        System.out.println("Loading library...");
        System.load(args[0]);
        Thread.sleep(2000);
        System.out.println("Loaded");

        System.out.println("[HOOKED]");
        stress();

        System.out.println("[*] Finished Performance Test");
    }
}
