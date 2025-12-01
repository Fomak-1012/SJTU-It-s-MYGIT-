# ***SJTUDream! It's MyGIT! ! ! ! !***

****一言以蔽之：类的实现就是一种封传箱子****

Fomak你记一下，我做如下部署调整：以Blob、Commit、StagingArea三个类为基础，共同实现一个gitlite的基础部件，再将Repository类的实现分散到RepositoryCore、StatusManager、BranchManager、CommitManager、MergeManager、RemoteManager中，同时利用好FileOperation、Utils和GitliteException简化文件和报错方面的操作，最后把类全部集成的Repository类中，并通过GitObj实现对外的接口。

## 大致骨架（会有部分交叉）
```text
src/
├── utils/                        
│   ├── Utils.h
│   ├── GitliteException.h
│   └── FileOperation.h
└── basis/                            
    ├── Blob.h                    # 文件快照的基础实现
    ├── Commit.h                  # 提交的基础实现
    ├── StagingArea.h             # 暂存区的基础实现
    └── Repository.h                       
        ├── RepositoryCore.h      # 仓库的核心管理
        ├── StatusManager.h       # 状态查询的实现
        ├── BranchManager.h       # 分支的基础管理
        ├── CommitManager.h       # 提交的基础管理
        ├── MergeManager.h        # 合并操作的重点实现
        └── RemoteManager.h       # 远程仓库操作的重点实现
```

### 以下是类的具体实现
