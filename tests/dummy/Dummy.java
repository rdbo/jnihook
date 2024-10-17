package dummy;

import java.io.IOException;

class Target {
    public static Target newTarget() {
        return new Target();
    }

    public static Target returnTarget(Target t) {
        return t;
    }

    public void sayHello() {
        System.out.println("Hello from Target object!");
    }
}

public class Dummy {
    public static void main(String[] args) throws IOException {
        System.out.println();
        System.out.println("Press [ENTER] to begin testing...");
        System.out.println("(wait for the library to load!)");
        System.out.println();

        if (args.length == 0) {
            System.out.println("Missing library path!");
            System.exit(-1);
        } else {
            System.out.println("Loading library...");
            System.load(args[0]);
        }

        System.in.read();

        Target target = Target.returnTarget(Target.newTarget());
        target.sayHello();

        System.out.println("Done!");
    }
}
