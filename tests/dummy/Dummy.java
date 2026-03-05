package dummy;

import java.io.IOException;

class Target {
    public static class TargetSubclass {
        public static void doWhatever() {
            System.out.println("do whatever");
        }
    }

    public static Target newTarget() {
        return new Target();
    }

    public static Target returnTarget(Target t) {
        return t;
    }

    public void say(String msg) {
        System.out.println(msg);
    }

    public void sayAnotherThing(int number) {
        this.say("Another thing from Target object!");
        this.say("My number: " + number);
    }

    public void sayHello() {
        Runnable a = new Runnable() {
            public void run() {
                System.out.println("runnable here");
            }
        };
        a.run();
        TargetSubclass.doWhatever();

        // Force bad case for jnihook
        Object obj = (Object)this;
        Target obj2 = (Target)obj;

        obj2.say("Hello from Target object!");

        this.sayAnotherThing(10);
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
        target.sayHello();

        System.out.println("Done!");
    }
}
