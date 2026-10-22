class Animation{
	public:
		const uint32_t* animation;
		uint32_t numFrames;
		uint32_t arrayLength;
		uint32_t Width;
		uint32_t Height;

		Animation(const uint32_t* a):animation(a){
			numFrames = animation[0];
			arrayLength = animation[1];
			Width = animation[2];
			Height = animation[3];
		}
};

