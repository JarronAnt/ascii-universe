#pragma once

namespace ascii {
   
    //X, Y Pos value initalized at 0 
    // TODO: add Z-axis
    struct Position {
        int x{};
        int y{};

        friend bool operator==(const Position&,const Position&) = default;
    };

}
