function PlotMesh()
% tau is a string of 18 bits. Each pair, in sequence, 
% determines a height on our 3x3 grid. 
% for tau = 0:(2^2-1),
  mesh = GenMesh(0, 10, 3, 3, false);
  surf(mesh.xx, mesh.yy, mesh.zz);  
  xlabel('x');
  ylabel('y');
  zlabel('z');
  axis('equal');
  waitforbuttonpress;
% end
end

