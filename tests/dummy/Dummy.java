package dummy;

public class Dummy {
    public static int myFunction(int mynumber, String name) {
        System.out.println("Welcome, " + name);
        System.out.println("Your number is: " + mynumber);
        return mynumber * mynumber;
    }
    
    public static void main(String[] args) {
        MyClass myObj = new MyClass("myObj");
        System.out.println("Started Dummy Process");

        if (args.length == 0) {
            System.out.println("Missing library path!");
        } else {
            System.out.println("Loading library...");
            System.load(args[0]);
        }

        while (true) {
            int result = myFunction(10, "Dummy");
            System.out.println("Result: " + result);
            System.out.println("MyObject: ");
            myObj.printName(3);

            try {
                Thread.sleep(1000);
            } catch (Exception e) {
                System.out.println("An unexpected error happened");
            }
        }
    }
}
