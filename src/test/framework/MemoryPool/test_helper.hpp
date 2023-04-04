class BlockBase
{
public:

};

template<size_t BlockSize>
class Block : public BlockBase
{
public:
	Block()
	{
		say("Block Created");
	}
	Block(char value)
	{
		say("Block Created by %c", value);
	}
	~Block()
	{
		say("Block Released");
	}

	char buffer[BlockSize - 32];
};