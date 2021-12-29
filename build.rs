use std::path::Path;

fn main() {
	match pkg_config::find_library("vorbis-encoder") {
		Ok(_) => return,
		Err(..) => {}
	};

	println!("{:?}", std::env::var("DEP_VORBIS_INCLUDE"));

	println!("cargo:rustc-flags=-l vorbis -l ogg");

	let inc1 = std::env::var("DEP_VORBIS_INCLUDE").unwrap();
	let inc2 = std::env::var("DEP_VORBIS_SRC").unwrap();
	let inc3 = std::env::var("DEP_OGG_INCLUDE").unwrap();

	cc::Build::new()
		.file("libvorbis-encoder/vorbis-encoder.c")
		.define("_USRDLL", None)
		.define("LIBVORBIS_EXPORTS", None)
		.include(&Path::new(&inc1))
		.include(&Path::new(&inc2))
		.include(&Path::new(&inc3))
		.compile("libvorbis-encoder.a");
}
