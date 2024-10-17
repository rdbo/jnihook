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
        System.out.print("Press [ENTER] to begin running...");
        System.out.flush();
        System.in.read();

        Target target = Target.returnTarget(Target.newTarget());
        target.sayHello();
    }
}
