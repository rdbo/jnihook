package dummy;

interface Thing {
    void doNothing();
}

public class Dummy implements Thing {
    public static void main(String[] args) throws InterruptedException {
        System.out.println("Started Dummy Process");

        if (args.length == 0) {
            System.out.println("Missing library path!");
        } else {
            System.out.println("Loading library...");
            System.load(args[0]);
        }
    }

    public void doNothing() {
        
    }
}
