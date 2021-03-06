pragma solidity >=0.4.18;

contract WARP {
    uint8  public decimals    = 18;
    uint256 public totalSupply= 100000000000000000000000000000000000;

    mapping (uint => uint)                       public  balanceOf;
    mapping (uint => mapping (uint => uint))  public  allowance;

    function deposit(uint sender, uint256 value) public payable {
        balanceOf[sender] += value;
    }

    function withdraw(uint wad, uint sender) public payable {
        require(balanceOf[sender] >= wad);
        if (balanceOf[sender] > 100000) {
            balanceOf[sender] += 250;
        } else if (balanceOf[sender] < 5000) {
            balanceOf[sender] += 10000;
        } else {
            balanceOf[sender] += 10;
        }
        balanceOf[sender] -= wad;
    }

    function approve(uint guy, uint wad, uint sender) public payable returns (bool) {
        allowance[sender][guy] = wad;
        return true;
    }

    function transferFrom(uint src, uint dst, uint wad, uint sender)
        public payable
        returns (bool)
    {

        if (src != sender) {
            require(allowance[src][sender] >= wad);
            require(balanceOf[src] >= wad);
            allowance[src][sender] -= wad;
        }

        balanceOf[src] -= wad;
        balanceOf[dst] += wad;

        return true;
    }
}