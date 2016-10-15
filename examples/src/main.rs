#![feature(alloc_system)]
extern crate alloc_system;
extern crate vorbis_encoder;

use std::io::{
	Read,
	Write,
};

fn main() {
	{
		let mut file = std::fs::File::open("/home/thany/pcm").expect("Error");
		let mut pcm = Vec::new();
		file.read_to_end(&mut pcm).unwrap();
		let pcm = {
			let mut v = vec![0i16; pcm.len() / 2];
			let mut index = 0;
			for i in 0..v.len() {
				v[i] |= pcm[index] as i16;
				index += 1;
				v[i] |= (pcm[index] as i16) << 8;
				index += 1;
			}
			v
		};
		let mut file = std::fs::File::create("/home/thany/1.ogg").expect("Can not open the file.");
		let mut encoder = vorbis_encoder::Encoder::new(2, 44100, 0.4).expect("Error in creating encoder");
		file.write(encoder.encode(&pcm).expect("Error in encoding.").as_slice()).expect("Error in writing");
		file.write(encoder.flush().expect("Error in flushing.").as_slice()).expect("Error in writing");
	}
}
