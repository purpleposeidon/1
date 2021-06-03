extern crate lib1;

fn main() {
    let mut stdin = lib1::stdin();
    let mut buf = vec![];
    //let mut stdout = lib1::stdout();
    while let Ok(p) = stdin.read(&mut buf) {
        if p.is_empty() { break; }
        println!("{:?}", p);
        //let f = format!("{:?}\n", p);
        //lib1::fmt!(FORMAT:f).write(&mut stdout).unwrap();
    }
}
